#include "DxLib.h"
#include <string>
#include <vector>
#include<functional>
#include<time.h>
#include<math.h>
using namespace std;
// 定数 const を付けると定数 
const int WINDOW_SIZE_X = 640;
const int WINDOW_SIZE_Y = 480;
const int BAR_SIZE_X = 100;
const int BAR_SIZE_Y = 5;
const int BALL_SIZE = 4;
const int BLOCK_SIZE_X = 40;
const int BLOCK_SIZE_Y = 20;
const int BLOCK_NUM_X = WINDOW_SIZE_X / BLOCK_SIZE_X;
const int BLOCK_NUM_Y = 6;
const int BLOCK_TOP_Y = 40;

// グローバル変数
class status {
	public:
		float x;
		float y;
		float vx;
		float vy;



};
status bar = { WINDOW_SIZE_X / 2, WINDOW_SIZE_Y * 9 / 10, 1, 1 };

struct ball {
	float x;
	float y;
	float vx;
	float vy;
};
vector<ball> balls;
// ボールの初期化
void InitBall()
{
	balls.clear();

	ball b;
	b.x = bar.x;
	b.y = bar.y - BALL_SIZE;
	b.vx = 1;
	b.vy = -1;

	balls.push_back(b);
}

int score = 0;//スコア
int nextAugmentScore = 500;
int block[BLOCK_NUM_Y][BLOCK_NUM_X] =
{
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
};
int BlockHandle[BLOCK_NUM_Y];//ブロックのビットマップ画像ハンドル
int BlockHitMusicHandle;
int WallHitMusicHandle;
int augmentCount = 0;//もっているオーグメントの数
int stage = 1;

enum GameMode {
	MODE_TITLE,          
	MODE_GAME,
	MODE_AUGMENT_SELECT,
	MODE_STAGE_CLEAR
};

GameMode mode = MODE_TITLE;

struct GameState {
	float speedMultiplier = 1.0f;
	float barScale = 1.0f;
	float ballScale = 1.0f;
	float barSpeedMultiplier = 1.0f;
	bool isPierce = false;
	bool hasAngleControl = false;
};
GameState game;

enum rarity {
	COMMON,
	RARE
};

class Augment {
public:
	string name;
	string description;
	function<void()> applyEffect;
	bool isUnique;
};

vector<Augment> currentChoices;

vector<Augment> CommonAugments = {
	{
		"Speed Boost",
		"ボール速度を20%増加",
		[]() {
			game.speedMultiplier *= 1.2f;
		},
		false
	},
	{
		"Wide Bar",
		"バーサイズを20%増加",
		[]() {
			game.barScale *= 1.2f;
		},
		false
	},
	{
		"Ball Size Up",
		"ボールサイズを50%増加",
		[]() {
			game.ballScale *= 1.5f;
		},
		false
	},
	{
		"Speed Down",
		"ボール速度を20%減少",
		[]() {
			game.speedMultiplier *= 0.8f;
		},
		false
	},
	{
		"Bar Speed Up",
		"バーの移動速度を20%増加",
		[]() {
			game.barSpeedMultiplier *= 1.2f;
		},
		false
	}
};

vector<Augment> RareAugments = {
	{
	"Angle Control",
	"バーの当たった位置で反射角が変わる",
	[]() {
		game.hasAngleControl = true;
	},
		true
	},
	{
	"Copy Ball",
	"ボールが2つに分裂",
	[]() {
		vector<ball> newBalls;

		for (auto& b : balls)
		{
			ball b1 = b;
			// 少し角度ずらす
			b1.vx += 0.5f;

			newBalls.push_back(b1);
		}

		// 元のボールに追加
		for (auto& nb : newBalls)
			balls.push_back(nb);
	},
	false
	},
	{
	"Add Ball",
	"ボールが3コ追加",
	[]() {
		for (int i = 0; i < 3; i++)
		{
			ball b;
			b.x = bar.x - (i - 1) * BALL_SIZE;
			b.y = bar.y - BALL_SIZE;
			b.vx = (float)(i - 1); //散らす
			b.vy = -1;
			balls.push_back(b);
		}
	},
	false
	},
	{
	"Pierce",
	"ボールがブロックを貫通する",
	[]() {
		game.isPierce = true;

	},
	true
	}
};
// ステージクリアの判定
bool IsStageClear()
{
	for (int y = 0; y < BLOCK_NUM_Y; y++)
	{
		for (int x = 0; x < BLOCK_NUM_X; x++)
		{
			if (block[y][x] == 1)
				return false;
		}
	}
	return true;
}
// ステージクリア画面の描画
void DrawStageClear()
{
	DrawBox(0, 0, WINDOW_SIZE_X, WINDOW_SIZE_Y, GetColor(0, 0, 0), TRUE);

	DrawString(200, 200, "STAGE CLEAR!", GetColor(255, 255, 0));
	DrawString(150, 260, "スペースで次へ", GetColor(255, 255, 255));
}
// ステージの初期化
void InitStage()
{
	for (int y = 0; y < BLOCK_NUM_Y; y++)
	{
		for (int x = 0; x < BLOCK_NUM_X; x++)
		{
			block[y][x] = 1;
		}
	}

	// ステージごとの差
	if (stage >= 2)
	{
		for (int x = 0; x < BLOCK_NUM_X; x += 2)
		{
			block[2][x] = 0; // 隙間
		}
	}

	if (stage >= 3)
	{
		for (int y = 0; y < BLOCK_NUM_Y; y++)
		{
			block[y][y % BLOCK_NUM_X] = 0; // 斜め
		}
		for (int y = BLOCK_NUM_Y-1; y >= 0; y--)
		{
			block[y][y % BLOCK_NUM_X] = 0; // 斜め
		}
	}
}
// ステージクリア画面の入力処理
void UpdateStageClear()
{
	if (CheckHitKey(KEY_INPUT_SPACE))
	{
		stage++;
		InitStage();
		mode = MODE_GAME;
	}
}

// タイトル画面の描画
void DrawTitle()
{
	DrawBox(0, 0, WINDOW_SIZE_X, WINDOW_SIZE_Y, GetColor(0, 0, 0), TRUE);

	DrawString(150, 150, "Augment Breaker", GetColor(255, 255, 255));
	DrawString(150, 220, "← → で移動", GetColor(200, 200, 200));
	DrawString(150, 250, "スコア到達で強化を選択", GetColor(200, 200, 200));
	DrawString(150, 280, "スペースでスタート", GetColor(200, 200, 200));
}
// タイトル画面の入力処理
void UpdateTitle()
{
	if (CheckHitKey(KEY_INPUT_SPACE))
	{
		mode = MODE_GAME;
	}
}
// 重複不可オーグメントが既に取得されているかどうかを判定する関数
bool IsOwned(const Augment& aug)
{
	if (!aug.isUnique) return false;

	if (aug.name == "Angle Control" && game.hasAngleControl)
		return true;
	if (aug.name == "Pierce" && game.isPierce)
		return true;

	return false;
}
// オーグメント選択
void GenerateAugments(rarity r)
{
	currentChoices.clear();
	int choices[3];
	do {
		choices[0] = rand() % CommonAugments.size();
	} while (IsOwned(CommonAugments[choices[0]]));
	do {
				choices[1] = rand() % CommonAugments.size();
	} while (choices[0] == choices[1] || IsOwned(CommonAugments[choices[1]]));
	do {
		choices[2] = rand() % CommonAugments.size();
	} while (choices[2] == choices[0] || choices[2] == choices[1] || IsOwned(CommonAugments[choices[2]]));
	if (r == RARE)//レアオーグメントを1つ混ぜる
	{
		do {
			choices[0] = rand() % RareAugments.size();
		} while (IsOwned(RareAugments[choices[0]]));
		currentChoices.push_back(RareAugments[choices[0]]);
		currentChoices.push_back(CommonAugments[choices[1]]);
		currentChoices.push_back(CommonAugments[choices[2]]);
		return;
	}
	currentChoices.push_back(CommonAugments[choices[0]]);
	currentChoices.push_back(CommonAugments[choices[1]]);
	currentChoices.push_back(CommonAugments[choices[2]]);
}
// オーグメント選択画面の描画
void DrawAugmentSelect()
{
	DrawBox(100, 100, 540, 380, GetColor(0, 0, 0), TRUE);

	for (int i = 0; i < static_cast<int>(currentChoices.size()); ++i)
	{
		int y = 150 + i * 60;

		DrawString(120, y,
			("[" + to_string(i + 1) + "] " + currentChoices[i].name).c_str(),
			GetColor(255, 255, 255));

		DrawString(120, y + 20,
			currentChoices[i].description.c_str(),
			GetColor(200, 200, 200));
	}
}
// オーグメント選択の入力処理
void UpdateAugmentSelect()
{	
	if (CheckHitKey(KEY_INPUT_1))
	{
		currentChoices[0].applyEffect();
		mode = MODE_GAME;
		
	}
	if (CheckHitKey(KEY_INPUT_2))
	{
		currentChoices[1].applyEffect();
		mode = MODE_GAME;
		
	}
	if (CheckHitKey(KEY_INPUT_3))
	{
		currentChoices[2].applyEffect();
		mode = MODE_GAME;
		
	}
	
}

//描画関数
void Draw()
{
	static int GrHandle = LoadGraph( "back.bmp" );//背景画像登録 640x480
	
	DrawGraph( 0 , 0 , GrHandle , FALSE );//背景を描く
	DrawBox(bar.x - BAR_SIZE_X*game.barScale / 2, bar.y,bar.x + BAR_SIZE_X*game.barScale / 2, bar.y + BAR_SIZE_Y,GetColor(255, 255, 255), TRUE);
	for (auto& ball : balls)
	{
		DrawCircle(ball.x, ball.y, BALL_SIZE * game.ballScale, GetColor(100, 100, 100), TRUE);
	}
	for ( int y = 0; y < BLOCK_NUM_Y; y++ )
	{
		for ( int x = 0; x < BLOCK_NUM_X; x++ )
		{
			if( block[y][x] )
				DrawGraph(x * BLOCK_SIZE_X, BLOCK_TOP_Y + y * BLOCK_SIZE_Y, BlockHandle[y], FALSE);
		}
	}
	DrawString(5, 5, ("STAGE: "+ to_string(stage) +  "   SCORE: " + to_string(score) + "   NEXT AUGMENT: " + to_string(nextAugmentScore)).c_str(), GetColor(255, 255, 0));
}

//Barの座標を変える関数
void MoveBar()
{
	if ( CheckHitKey( KEY_INPUT_RIGHT ) &&  bar.x+BAR_SIZE_X*game.barScale/2 < WINDOW_SIZE_X) // → キー を押したか
	{
		bar.x += game.barSpeedMultiplier;
	}
	else if (CheckHitKey(KEY_INPUT_LEFT) && bar.x-BAR_SIZE_X*game.barScale/2 > 0)
	{
		bar.x -= game.barSpeedMultiplier;
	}

}

//画面上のX座標　bxからブロックのX軸番号を計算して返す
int getBlockNumX(int bx)
{
	return bx / BLOCK_SIZE_X;
}
//画面上のY座標　byから　ブロックのY軸番号を計算して返す
//該当なしなら-1を返す
int getBlockNumY(int by)
{
	if (by >= BLOCK_TOP_Y && by < BLOCK_TOP_Y + BLOCK_NUM_Y * BLOCK_SIZE_Y)
		return (by - BLOCK_TOP_Y) / BLOCK_SIZE_Y;
	else
		return -1;
}
//ボールとブロックが当たったかどうかを返す関数
bool isDeleteBlock(int bx, int by)
{
	int BlockNumY = getBlockNumY(by);
	int BlockNumX = getBlockNumX(bx);
	if (BlockNumY != -1)
	{
		if (block[BlockNumY][BlockNumX] == 1)
		{
			block[BlockNumY][BlockNumX] = 0;
			score += 100;
			return true;
		}
	}
	return false;
}
//ボールの座標を変える関数 反射を考える
void MoveBalls()
{
	for (auto& ball : balls)
	{
		float radius = BALL_SIZE * game.ballScale;

		float ballx1 = ball.x - radius;
		float ballx2 = ball.x + radius;
		float bally1 = ball.y - radius;
		float bally2 = ball.y + radius;
		bool hitWall = false;
		bool hitBlock = false;
		// 壁
		if (ballx1 <= 0)
		{
			ball.vx = -ball.vx;
			ball.x = radius; // 左端に戻す
			hitWall = true;
		}
		else if (ballx2 >= WINDOW_SIZE_X)
		{
			ball.vx = -ball.vx;
			ball.x = WINDOW_SIZE_X - radius; // 右端に戻す
			hitWall = true;
		}


		if (bally1 <= 0)
		{
			ball.vy = -ball.vy;
			hitWall = true;
		}


		if (bally2 >= WINDOW_SIZE_Y)//テスト用
			ball.vy = -ball.vy;

		// バー
		float barHalf = (BAR_SIZE_X * game.barScale) / 2.0f;

		if (bally2 >= bar.y && bally1 < bar.y &&
			ballx2 >= bar.x - barHalf &&
			ballx1 <= bar.x + barHalf)
		{
			if (game.hasAngleControl)
			{
				float hitPos = (ball.x - bar.x) / barHalf;

				ball.vx = hitPos * 2.0f;
				ball.vy = -fabs(ball.vy);

			}
			else
			{
				ball.vy = -fabs(ball.vy);
			}

			ball.y = bar.y - radius;
			hitWall = true;
		}

		// ブロック
		bool hitX = isDeleteBlock(ballx1, ball.y) || isDeleteBlock(ballx2, ball.y);
		bool hitY = isDeleteBlock(ball.x, bally1) || isDeleteBlock(ball.x, bally2);

		if (!game.isPierce)
		{
			if (hitX)
			{
				ball.vx = -ball.vx;
				hitBlock = true;
			}
			if (hitY)
			{
				ball.vy = -ball.vy;
				hitBlock = true;
			}
		}
		else
		{
			// 貫通時：壊すだけ（反射なし）
			if (hitX || hitY)
			{
				hitBlock = true;
			}
		}


		// 正規化
		float baseSpeed = 1.0f;
		float len = sqrt(ball.vx * ball.vx + ball.vy * ball.vy);

		if (len != 0)
		{
			ball.vx = (ball.vx / len) * baseSpeed * game.speedMultiplier;
			ball.vy = (ball.vy / len) * baseSpeed * game.speedMultiplier;
		}


		// 移動
		ball.x += ball.vx;
		ball.y += ball.vy;

		if (hitBlock)
		{
			PlaySoundMem(BlockHitMusicHandle, DX_PLAYTYPE_BACK);

		}
		else if (hitWall)
		{
			PlaySoundMem(WallHitMusicHandle, DX_PLAYTYPE_BACK);
		}
	}
}


// WinMain関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	srand((unsigned int)time(NULL));
	// タイトルを 変更
	SetMainWindowText("ブロック崩し");
	// ウインドウモードに変更
	ChangeWindowMode(TRUE);
	// ＤＸライブラリ初期化処理
	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
		return -1;			// エラーが起きたら直ちに終了
	// マウスを表示状態にする
	SetMouseDispFlag(TRUE);


	
	//int BGMHandle = LoadSoundMem("wa_001.wav");//BGMの読み込み
	int BGMHandle = LoadSoundMem("PicoDash!!.mp3");//BGMの読み込み
	BlockHitMusicHandle = LoadSoundMem("putMAN.wav");
	WallHitMusicHandle = LoadSoundMem("putCOM.wav");
	// ＢＭＰ画像のメモリへの分割読み込み
	LoadDivGraph("block.bmp", 6, 6, 1, BLOCK_SIZE_X, BLOCK_SIZE_Y, BlockHandle);
	
	InitBall();
	Draw();
	PlaySoundMem(BGMHandle, DX_PLAYTYPE_LOOP);//BGMの再生

	do
	{
		DrawTitle();
		UpdateTitle();
	} while (mode == MODE_TITLE);


	while (1)//ゲームを何度もプレイするためのループ
	{
		// スペースボタンが押されるまで待機
		while (!CheckHitKey(KEY_INPUT_SPACE))
		{
			// メッセージループに代わる処理をする
			if (ProcessMessage() == -1)
			{
				DxLib_End();				// ＤＸライブラリ使用の終了処理
				return 0;				// ソフトの終了 
			}
		}


		// ゲームメインループ
		while (1)
		{
			// メッセージループに代わる処理をする
			if (ProcessMessage() == -1)
			{
				DxLib_End();				// ＤＸライブラリ使用の終了処理
				return 0;				// ソフトの終了 
			}
			if (mode == MODE_TITLE)
			{
				DrawTitle();
				UpdateTitle();
			}
			else if (mode == MODE_GAME)
			{
				MoveBar();
				MoveBalls();
				Draw();
				if (augmentCount == 0)
				{
					GenerateAugments(RARE);
					mode = MODE_AUGMENT_SELECT;
					augmentCount++;
				}
				// テスト：スコアで遷移
				if (score >= nextAugmentScore)
				{
					if (augmentCount % 2 == 1)
						GenerateAugments(RARE);
					else
						GenerateAugments(COMMON);

					mode = MODE_AUGMENT_SELECT;
					augmentCount++;

					// 次回必要スコアを増加
					nextAugmentScore *= 1.5f;
				}
				
				if (IsStageClear())
				{
					mode = MODE_STAGE_CLEAR;
				}

			}
			else if (mode == MODE_STAGE_CLEAR)
			{
				DrawStageClear();
				UpdateStageClear();
			}
			else if (mode == MODE_AUGMENT_SELECT)
			{
				DrawAugmentSelect();
				UpdateAugmentSelect();
			}
			


			WaitTimer(4);
			
		}
		
	}
	DxLib_End() ;				// ＤＸライブラリ使用の終了処理
	return 0;				// ソフトの終了 
}