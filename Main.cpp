#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstdlib>
#include <thread>
#include "Math.h"

namespace Math {
	using namespace matharz;
};
#undef matharz

#define TEST_MACHINERY 0
struct GameContext
{
	int width;
	int heigth;
	int EnemyShipsCount;
	int MissileShipDetectionRadius;
	int EnemyShipsDestroyed;
	bool IsGameWon = false;
	bool shouldExit = false;
	int currentTargetID = -1;
	bool isCurrentTargetDestroyed = false;

	bool SkipInputCheck = false;

	int CurrentCellID = -1;
	int CurrentCellEnemies = -1;
	bool CurrentCellMissileCheck = false;

	struct StrikeResult {
		bool Hit = false;
		int EnemiesCount = -1;
		std::vector<float> EnemiesDistances{};
		Math::vec2i StrikePos{};
		bool valid = false;
		bool NeedUpdate = false;
	};

	std::array<StrikeResult, 3> LastThreeStrikeResults;
	std::array<StrikeResult, 5> FireShotsHistroy;
	int LastStrikeID = -1;
	int LastFireShotID = -1;
	bool ShootCoordsPrepared = false;
	bool LastCellInMap = false;
	bool AddNoiseForPointsLocations = false;
	std::vector<Math::vec2i> ShootCoords;
};

#if(TEST_MACHINERY == 1)
#include <mutex>
const int MaxMSGSIZE = 10000;
static char CharMSGCOMMANDBUFF[2][MaxMSGSIZE];
static int LastMSGCOMMANDBUFFPos[2] = { 0,0 };
static std::mutex CHAR_MSG_MUTEX{};

int readInputConsole(int bufferLength, char* buffer);

enum MSGBUFF_THREAD
{
	TEST = 0,
	GAME = 1
};

void WriteMSGToCommandBuff(char* MsgSTR, MSGBUFF_THREAD BUFF)
{
	std::lock_guard<std::mutex>{ CHAR_MSG_MUTEX };
	int MsgLEN = strlen(MsgSTR);
	int i = LastMSGCOMMANDBUFFPos[BUFF];
	int ExtendedMSGLength = i + MsgLEN;
	LastMSGCOMMANDBUFFPos[BUFF] += MsgLEN;
	while (i < ExtendedMSGLength)
	{
		CharMSGCOMMANDBUFF[BUFF][i] = MsgSTR[i];
		i++;
	}
}

void WriteMSGToCommandBuff(std::string MsgSTR, MSGBUFF_THREAD BUFF)
{
	std::lock_guard<std::mutex>{ CHAR_MSG_MUTEX };
	int MsgLEN = MsgSTR.length();
	int i = LastMSGCOMMANDBUFFPos[BUFF];
	int ExtendedMSGLength = i + MsgLEN;
	LastMSGCOMMANDBUFFPos[BUFF] += MsgLEN;
	int SrcIndex = 0;
	while (SrcIndex < MsgLEN)
	{
		CharMSGCOMMANDBUFF[BUFF][i] = MsgSTR[SrcIndex];
		SrcIndex++;
		i++;
	}
}

std::string ReadMSGFromCommandBuff(MSGBUFF_THREAD BUFF)
{

	std::lock_guard<std::mutex>{ CHAR_MSG_MUTEX };
	std::string Result{};
	Result.append(CharMSGCOMMANDBUFF[BUFF], LastMSGCOMMANDBUFFPos[BUFF]);
	//memset(CharMSGCOMMANDBUFF[BUFF], (int)' ', LastMSGCOMMANDBUFFPos[BUFF]);
	LastMSGCOMMANDBUFFPos[BUFF] = 0;
	return Result;
}
// Does not change lastmsgcharposs
std::string LootAtMSGFromCommandBuff(MSGBUFF_THREAD BUFF)
{
	std::lock_guard<std::mutex>{ CHAR_MSG_MUTEX };
	std::string Result{};
	Result.append(CharMSGCOMMANDBUFF[BUFF], LastMSGCOMMANDBUFFPos[BUFF]);
	return Result;
}

#endif

static GameContext G_gameContext{};

void MakeGameActions();
bool IsPointInCell(Math::vec2i Point, Math::vec2i CellStartPos, int radius);
Math::vec2i GetRandomPointInCell(Math::vec2i CellStartPos, int radius);

Math::vec2i GetRandomPointInCellUniformly(Math::vec2i CellStartPos, int radius);

void MakeGameActions();
void ParseEntryConsoleCommands(int argcs, char* argv);
int ParseConsoleArgumentINT(int& refValue, char* argv);
int ParseConsoleArgumentFLOAT(float& refValue, char* argv);
#if(TEST_MACHINERY == 1)
//return last readed pos index
int readInput(int bufferLength, char* buffer, MSGBUFF_THREAD BUFF);
#else
int readInput(int bufferLength, char* buffer);
#endif
std::string GetStringFromPos(Math::vec2i Pos);

const int GetBorderCellLength()
{
	int CellBorderLength = G_gameContext.MissileShipDetectionRadius;

	CellBorderLength = CellBorderLength > G_gameContext.width ? G_gameContext.width :
		CellBorderLength > G_gameContext.heigth ? G_gameContext.heigth :
		CellBorderLength;
	return CellBorderLength;
}

enum GameConsoleMSGArgType
{
	MSG,
	intArg,
	floatArg
};

struct ParsedGameArgument
{
	bool isMessage = false;
	bool Hit = false;
	bool Miss = false;
	bool Win = false;
	int valueInt;
	float valueFloat;
};

int ParseGameConsoleArgument(GameConsoleMSGArgType msgType, char* buff, ParsedGameArgument& gameArg);

#if(TEST_MACHINERY == 1)
bool ParseGameInput(MSGBUFF_THREAD BUFF);
#else
bool ParseGameInput();
#endif

GameContext::StrikeResult& FindLastMissileCheckResult()
{
	if (G_gameContext.CurrentCellMissileCheck)
	{
		if (G_gameContext.LastStrikeID == -1)
			return G_gameContext.LastThreeStrikeResults[0];
		else
			return G_gameContext.LastThreeStrikeResults[G_gameContext.LastStrikeID];
	}
};
GameContext::StrikeResult& FindLastFireShotHistroyResult()
{
	if (G_gameContext.LastFireShotID == -1)
		return G_gameContext.FireShotsHistroy[0];
	else
		return G_gameContext.FireShotsHistroy[G_gameContext.LastFireShotID];
}

bool IsPointInCell(Math::vec2i Point, Math::vec2i CellStartPos, int radius)
{
	return Point >= CellStartPos && Point <= CellStartPos + GetBorderCellLength();
}

Math::vec2i GetRandomPointInCell(Math::vec2i CellStartPos, int radius)
{
	Math::vec2i RandomPos = CellStartPos + Math::vec2i{ rand() % GetBorderCellLength(), rand() % GetBorderCellLength() };
	return RandomPos;
}

Math::vec2i GetRandomPointInCellUniformly(Math::vec2i CellStartPos, int radius)
{
	bool FoundRandom = false;
	int iters = 0;
	bool ShouldExit = false;
	Math::vec2i RandomPosResult{};
	while (!FoundRandom || !ShouldExit)
	{
		auto RandomPoint = GetRandomPointInCell(CellStartPos, radius);
		bool IsGood = true;
		RandomPosResult = RandomPoint;
		for (auto StrikeResult : G_gameContext.LastThreeStrikeResults)
		{
			if (StrikeResult.valid && IsPointInCell(StrikeResult.StrikePos, CellStartPos, radius))
			{
				// used for determining if we found a "good" random point that is further then declared range(for now half of radius)
				float StrikeCancelRange = ((float)(radius)) * ((float)(radius));
				bool IsInCancelRange = (static_cast<Math::vec2>(StrikeResult.StrikePos) - static_cast<Math::vec2>(RandomPoint)).SquareLength() <=
					StrikeCancelRange;
				if (IsInCancelRange)
					IsGood = false;
			}
		}
		iters++;
		constexpr int MaxItersLoop = 10;
		// just return as it is, becuase we spend too much time in loop
		if (iters > MaxItersLoop)
			break;
		if (IsGood)
			FoundRandom = true;
	}

	if (G_gameContext.AddNoiseForPointsLocations)
	{
		RandomPosResult += Math::vec2i{ rand() % ((GetBorderCellLength() / 2) + 1), rand() % ((GetBorderCellLength() / 2) + 1) };
	};

	return RandomPosResult;
}

#if(TEST_MACHINERY == 1)
struct TestGameContext
{
	int EnemyCount = 0;
	int DestroyedEnemies = 0;
	int TestRadius = 0;
	Math::vec2i MapSize;
	struct EnemyData
	{
		Math::vec2i Pos;
		bool Destroyed = false;
	};
	std::vector<EnemyData> EnemiesPositions;
};
std::thread* TestThread = nullptr;

void TestGameMain()
{
	TestGameContext TestGameContext;
	bool Initialized = false;
	while (!Initialized)
	{
		static constexpr int charBuffMax = 10000;
		static char charBuff[charBuffMax];
		int length = 0;
		length += readInputConsole(charBuffMax, (char*)charBuff);
		// no input
		if (!length)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(15));
			continue;
		};

		{
			char* argvHead = charBuff;
			argvHead += ParseConsoleArgumentINT(TestGameContext.MapSize.x, argvHead);
			argvHead += ParseConsoleArgumentINT(TestGameContext.MapSize.y, argvHead);
			argvHead += ParseConsoleArgumentINT(TestGameContext.EnemyCount, argvHead);
			argvHead += ParseConsoleArgumentINT(TestGameContext.TestRadius, argvHead);
			TestGameContext.EnemiesPositions.reserve(TestGameContext.EnemyCount);

			for (int index = 0; index < TestGameContext.EnemyCount; index++)
			{
				bool IsGoodRandom = false;
				Math::vec2i EnemyRandomPos;
				while (!IsGoodRandom)
				{
					EnemyRandomPos = { ((rand() + 1) * rand()) % TestGameContext.MapSize.x, ((rand() + 1) * rand()) % TestGameContext.MapSize.y };
					for (int EnemiesIndex = 0; EnemiesIndex < TestGameContext.EnemiesPositions.size(); EnemiesIndex++)
					{
						if (EnemyRandomPos == TestGameContext.EnemiesPositions[EnemiesIndex].Pos)
						{
							IsGoodRandom = false;
							break;
						}
					}
					IsGoodRandom = true;
					std::cout << "[TEST MACHINERY] ENEMY POS: " << GetStringFromPos(EnemyRandomPos) << '\n';
				};
				TestGameContext.EnemiesPositions.push_back({ EnemyRandomPos ,false});
			};

		}
		Initialized = true;
	};

	{
		//std::cout << "s\n"; // Launch Game
		//std::cout << GetStringFromPos(TestGameContext.MapSize) << ' ' << std::to_string(TestGameContext.EnemyCount).c_str() 
		//	<< ' ' << std::to_string(TestGameContext.TestRadius) << '\n';
		std::string Msg = GetStringFromPos(TestGameContext.MapSize) + ' ' + std::to_string(TestGameContext.EnemyCount).c_str()
			+ ' ' + std::to_string(TestGameContext.TestRadius) + '\n';
		std::cout << "[TEST MACHINERY]: " << Msg.c_str() << '\n';
		WriteMSGToCommandBuff(Msg, TEST);
		bool ShouldGameExit = false;
		while (!ShouldGameExit)
		{
			static constexpr int charBuffMax = 10000;
			static char charBuff[charBuffMax];

			int length = 0;
			length += readInput(charBuffMax, (char*)charBuff, GAME);
			// no input
			if (!length)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(20)); continue;
			};

			Math::vec2i ShootPos{};
			int overrallOffset = 0;
			ParsedGameArgument XArg;
			ParsedGameArgument YArg;
			overrallOffset += ParseGameConsoleArgument(intArg, ((char*)(charBuff)+overrallOffset), XArg);
			overrallOffset += ParseGameConsoleArgument(intArg, ((char*)(charBuff)+overrallOffset), YArg);
			ShootPos = { XArg.valueInt, YArg.valueInt };

			std::vector<float> Distances{};
			bool Hit = false;
			int HitEnemyIndex = 0;
			for (auto& Enemy : TestGameContext.EnemiesPositions)
			{
				if (Enemy.Destroyed)
					continue;

				if (Enemy.Pos == ShootPos)
				{
					Hit = true; Enemy.Destroyed = true;
					TestGameContext.DestroyedEnemies++;
					break;
				};

				float Distance = ((Math::vec2)ShootPos - (Math::vec2)(Enemy.Pos)).Lenght();
				if (Distance <= TestGameContext.TestRadius)
				{
					Distances.push_back(Distance);
					HitEnemyIndex++;
				};

			}

			if (TestGameContext.DestroyedEnemies == TestGameContext.EnemyCount)
			{
				WriteMSGToCommandBuff("Win!\n", TEST);
				std::cout << "[TEST MACHINERY] Win!\n";
				ShouldGameExit = true;
			};

			if (Hit)
			{
				WriteMSGToCommandBuff("Hit!\n", TEST);
				std::cout << "[TEST MACHINERY] Hit!\n";
			}
			else
			{
				std::string MissStringBuff{};
				MissStringBuff.append("Miss! ");
				MissStringBuff.append(std::to_string(Distances.size()) + ' ');
				for (auto& Distance : Distances)
					MissStringBuff.append(std::to_string(Distance) + ' ');
				MissStringBuff.append(1, '\n');

				WriteMSGToCommandBuff(MissStringBuff, TEST);
				std::cout << "[TEST MACHINERY] " << MissStringBuff;
			}
			//std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
		std::cout << "[TEST MACHINE] [END SESSION]\n";
	}

};
#endif



int main(int argcs, char** argv)
{
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());

#if(TEST_MACHINERY == 1)
		TestThread = new std::thread(&TestGameMain);
#endif
	bool Initialized = false;
	while (!Initialized)
	{
		static constexpr int charBuffMax = 10000;
		static char charBuff[charBuffMax];

		int length = 0;
#if(TEST_MACHINERY == 1)
		length += readInput(charBuffMax, (char*)charBuff, TEST);
#else
		length += readInput(charBuffMax, (char*)charBuff);
#endif
		// no input
		if (!length)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(15));
			continue;
		};
		ParseEntryConsoleCommands(length, (char*)charBuff);
		Initialized = true;
		MakeGameActions();
	}

	while (!G_gameContext.IsGameWon || !G_gameContext.shouldExit)
	{
		if (!G_gameContext.SkipInputCheck)
		{
#if(TEST_MACHINERY == 1)
			if (!ParseGameInput(TEST))
			{
#else
			if (!ParseGameInput())
			{
#endif
				std::this_thread::sleep_for(std::chrono::milliseconds(40));
				continue;
			};
		}
		else
		{
			G_gameContext.SkipInputCheck = false;
		}

		if (G_gameContext.IsGameWon || G_gameContext.shouldExit)
			break;

		MakeGameActions();
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}
#if(TEST_MACHINERY == 1)
	TestThread->join();
	delete TestThread;
#endif
}

std::string GetStringFromPos(Math::vec2i Pos)
{
	std::string Result{};
	Result.append(std::to_string(Pos.x));
	Result.append(1, ' ');
	Result.append(std::to_string(Pos.y));
	return Result;
}

void MakeGameActions()
{
	if (G_gameContext.CurrentCellID == -1)
	{
		G_gameContext.CurrentCellID = 0;
		G_gameContext.CurrentCellMissileCheck = true;
	}
	
	int CellBorderLength = GetBorderCellLength();

	const float WidthCellsCount = (float)G_gameContext.width / (float)CellBorderLength;
	Math::vec2i CurrentCellCoords = { (int)(Math::min(Math::mod((float)G_gameContext.CurrentCellID, std::ceil(WidthCellsCount))
										* (float)CellBorderLength, G_gameContext.width)),
									  (int)(Math::min(std::ceil(std::ceil((float)G_gameContext.CurrentCellID / std::ceil(WidthCellsCount)-1.f) / WidthCellsCount)
										  * CellBorderLength, G_gameContext.heigth)) };

	if (CurrentCellCoords >= (Math::vec2i{ G_gameContext.width, G_gameContext.heigth } - CellBorderLength))
	{
		G_gameContext.LastCellInMap = true;
		G_gameContext.AddNoiseForPointsLocations = true;
	};

	if (G_gameContext.CurrentCellMissileCheck)
	{
		bool FinishedMissileCheck = G_gameContext.LastStrikeID + 1 >= G_gameContext.LastThreeStrikeResults.size();
		if (FinishedMissileCheck)
		{
#if(TEST_MACHINERY == 1)
		std::cout << "[GAME] [CELL MISSILE CHECK FINISHED] [CELL COORDS: " << GetStringFromPos(CurrentCellCoords) << " ]\n";
#endif
			G_gameContext.CurrentCellMissileCheck = false;
			G_gameContext.LastStrikeID = (G_gameContext.LastStrikeID + 1) % G_gameContext.LastThreeStrikeResults.size();
		};
	};

	if (G_gameContext.CurrentCellMissileCheck)
	{
		Math::vec2i ShootCoords{};
		ShootCoords = GetRandomPointInCellUniformly(CurrentCellCoords, G_gameContext.MissileShipDetectionRadius);
		if (G_gameContext.LastStrikeID == -1)
			G_gameContext.LastStrikeID = 0;
		else
			G_gameContext.LastStrikeID++;
		G_gameContext.LastThreeStrikeResults[G_gameContext.LastStrikeID].NeedUpdate = true;
		G_gameContext.LastThreeStrikeResults[G_gameContext.LastStrikeID].StrikePos = ShootCoords;
		//ShootCoords = { (rand()+1) * (rand() + 1) % G_gameContext.width ,}
#if(TEST_MACHINERY == 1)
		WriteMSGToCommandBuff(GetStringFromPos(ShootCoords) + '\n', GAME);

		std::cout << "[GAME]: " << GetStringFromPos(ShootCoords) << '\n';
#else
		std::cout << GetStringFromPos(ShootCoords).c_str() << '\n';
#endif
		return;
	}
	// Try to destroy all ships and predict about ships count in cell
	else
	{
		if (G_gameContext.ShootCoords.size())
		{
			auto StrikeCoord = G_gameContext.ShootCoords.back();
			G_gameContext.ShootCoords.pop_back();

			if (!G_gameContext.ShootCoords.size())
			{
				G_gameContext.ShootCoordsPrepared = false;
				G_gameContext.CurrentCellMissileCheck = true;
				G_gameContext.CurrentCellID++;
				if (G_gameContext.LastCellInMap)
				{
					G_gameContext.LastCellInMap = false;
					G_gameContext.CurrentCellID = -1;
				};
			};

			if (G_gameContext.LastFireShotID == -1)
				G_gameContext.LastFireShotID = 0;
			else
				G_gameContext.LastFireShotID = (G_gameContext.LastFireShotID + 1) % G_gameContext.FireShotsHistroy.size();

#if(TEST_MACHINERY == 1)
			WriteMSGToCommandBuff(GetStringFromPos(StrikeCoord) + '\n', GAME);
			std::cout << "[GAME]: " << GetStringFromPos(StrikeCoord).c_str() << '\n';
#else
			std::cout << GetStringFromPos(StrikeCoord).c_str() << '\n';
#endif
		}
		else
		{
			int StrikeIndex = 0;
			std::vector<Math::vec2i> PossibleTargetsPos;
			PossibleTargetsPos.reserve(5);

			for (auto& StrikeResult : G_gameContext.LastThreeStrikeResults)
			{

				if (StrikeResult.Hit || StrikeResult.EnemiesCount == 0)
					continue;

				for (int OtherStrikesIndexes = 0; OtherStrikesIndexes < G_gameContext.LastThreeStrikeResults.size(); OtherStrikesIndexes++)
				{
					if (OtherStrikesIndexes == StrikeIndex)
						continue;

					auto& OtherStrike = G_gameContext.LastThreeStrikeResults[OtherStrikesIndexes];
					for (int EnemyIndex = 0; EnemyIndex < StrikeResult.EnemiesCount; EnemyIndex++)
					{
						for (int OtherStrikeEnemyIndex = 0; OtherStrikeEnemyIndex < OtherStrike.EnemiesCount; OtherStrikeEnemyIndex++)
						{
							if (((OtherStrike.StrikePos) == (StrikeResult.StrikePos)))
								continue;
							// https://math.stackexchange.com/questions/2228018/how-to-calculate-the-third-point-if-two-points-and-all-distances-between-the-poi
							const auto AB_VEC = ((Math::vec2)(OtherStrike.StrikePos) - (Math::vec2)(StrikeResult.StrikePos));
							const auto A_VEC = StrikeResult.StrikePos;
							const auto B_VEC = OtherStrike.StrikePos;
							const auto AC_LEN = StrikeResult.EnemiesDistances[EnemyIndex];
							const auto BC_LEN = OtherStrike.EnemiesDistances[OtherStrikeEnemyIndex];
							const auto AB_LEN = AB_VEC.Lenght();
							const auto TriangleArea = 0.25 * Math::sqrt((BC_LEN + AC_LEN + AB_LEN) *
								(BC_LEN + AC_LEN - AB_LEN) * (BC_LEN - AC_LEN + AB_LEN) * (-BC_LEN + AC_LEN + AB_LEN));

							// NAN
							if (TriangleArea != TriangleArea)
								continue;

							if (StrikeResult.EnemiesDistances[EnemyIndex] <=
								OtherStrike.EnemiesDistances[OtherStrikeEnemyIndex] + AB_LEN)
							{
								// could be -+, but since map is in o + 10000000 range, don't bother about that
								const auto e = 1.0;
								const auto CalculatedPosX = A_VEC.x + ((AB_LEN * AB_LEN + AC_LEN * AC_LEN - BC_LEN * BC_LEN) / (2.0 * (AB_LEN * AB_LEN)))
									* (B_VEC.x - A_VEC.x) + e * ((2.0 * TriangleArea) / (AB_LEN * AB_LEN)) * (A_VEC.y - B_VEC.y);

								const auto CalculatedPosY = A_VEC.y + ((AB_LEN * AB_LEN + AC_LEN * AC_LEN - BC_LEN * BC_LEN) / (2.0 * (AB_LEN * AB_LEN)))
									* (B_VEC.y - A_VEC.y) + e * ((2.0 * TriangleArea) / (AB_LEN * AB_LEN)) * (A_VEC.x - B_VEC.x);

								const auto CalculatedPosX2 = A_VEC.x + ((AB_LEN * AB_LEN + AC_LEN * AC_LEN - BC_LEN * BC_LEN) / (2.0 * (AB_LEN * AB_LEN)))
									* (B_VEC.x - A_VEC.x) + -e * ((2.0 * TriangleArea) / (AB_LEN * AB_LEN)) * (A_VEC.y - B_VEC.y);

								const auto CalculatedPosY2 = A_VEC.y + ((AB_LEN * AB_LEN + AC_LEN * AC_LEN - BC_LEN * BC_LEN) / (2.0 * (AB_LEN * AB_LEN)))
									* (B_VEC.y - A_VEC.y) + -e * ((2.0 * TriangleArea) / (AB_LEN * AB_LEN)) * (A_VEC.x - B_VEC.x);
								// NAN
								if (CalculatedPosX != CalculatedPosX || CalculatedPosX2 != CalculatedPosX2
									|| CalculatedPosY != CalculatedPosY || CalculatedPosY2 != CalculatedPosY2)
								{
									continue;
								};

								//auto fractX1 = CalculatedPosX - (long)CalculatedPosX;
								//auto fractX2 = CalculatedPosX2 - (long)CalculatedPosX2;
								//if (fractX1 > 0.15 && fractX1 < .85 && 
								//	fractX2 > 0.15 && fractX2 < .85)
								//{
								//	// Noised Input
								//	continue;
								//};
								std::vector<Math::vec2i> PossiblrStrikePoses{};
								const float offsetGuard = .5;
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX + offsetGuard) , (int)Math::floor(CalculatedPosY + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX + offsetGuard) , (int)Math::floor(CalculatedPosY - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX - offsetGuard) , (int)Math::floor(CalculatedPosY + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 + offsetGuard) , (int)Math::floor(CalculatedPosY + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 + offsetGuard) , (int)Math::floor(CalculatedPosY - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 - offsetGuard) , (int)Math::floor(CalculatedPosY + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX + offsetGuard) , (int)Math::floor(CalculatedPosY2 + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX + offsetGuard) , (int)Math::floor(CalculatedPosY2 - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX - offsetGuard) , (int)Math::floor(CalculatedPosY2 + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 + offsetGuard) , (int)Math::floor(CalculatedPosY2 + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 + offsetGuard) , (int)Math::floor(CalculatedPosY2 - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 - offsetGuard) , (int)Math::floor(CalculatedPosY2 + offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX - offsetGuard) , (int)Math::floor(CalculatedPosY - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 - offsetGuard) , (int)Math::floor(CalculatedPosY - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX - offsetGuard) , (int)Math::floor(CalculatedPosY2 - offsetGuard) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2 - offsetGuard) , (int)Math::floor(CalculatedPosY2 - offsetGuard) });

								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX) , (int)Math::floor(CalculatedPosY2) });
								PossiblrStrikePoses.push_back({ (int)Math::floor(CalculatedPosX2) , (int)Math::floor(CalculatedPosY2) });

								for (auto& PossibleTargetPos : PossiblrStrikePoses)
								{
									bool Unique = true;
									bool Discard = false;
									bool ApprovedTestByDistance = false;
									if (PossibleTargetPos.x < 0 || PossibleTargetPos.x > G_gameContext.width
										|| PossibleTargetPos.y < 0 || PossibleTargetPos.y > G_gameContext.heigth)
									{
										Discard = true;
									};
									if (!Discard)
									{
										for (auto& PotentionalTarget : PossibleTargetsPos)
										{
											if (PotentionalTarget == PossibleTargetPos)
											{
												Unique = false;
												break;
											};
										};
									};

									if (Unique && !Discard)
									{
										const auto AC_TEST_VEC = Math::vec2(PossibleTargetPos) - (Math::vec2)StrikeResult.StrikePos;
										const auto BC_TEST_VEC = Math::vec2(PossibleTargetPos) - (Math::vec2)OtherStrike.StrikePos;
										ApprovedTestByDistance = Math::equal(AC_TEST_VEC.Lenght(), AC_LEN, 0.3f) && Math::equal(BC_TEST_VEC.Lenght(), BC_LEN, 0.3f);
									};

									if (!Discard && Unique && ApprovedTestByDistance)
									{
										PossibleTargetsPos.push_back(PossibleTargetPos);
									};
								};
							}
						};
					}
				};
				StrikeIndex++;
			}

			G_gameContext.ShootCoords = PossibleTargetsPos;
			G_gameContext.ShootCoordsPrepared = true;

			if (!StrikeIndex || G_gameContext.ShootCoords.size() <= 1)
			{
				G_gameContext.ShootCoordsPrepared = false;
				G_gameContext.CurrentCellMissileCheck = true;
				G_gameContext.CurrentCellID++;
				if (G_gameContext.LastCellInMap)
				{
					G_gameContext.LastCellInMap = false;
					G_gameContext.CurrentCellID = -1;
				};
				G_gameContext.SkipInputCheck = true;
			};

			if (G_gameContext.ShootCoords.size())
			{
#if(TEST_MACHINERY == 1)
				{
					std::string StrikeCoordsSTRPRintMSG{};
					StrikeCoordsSTRPRintMSG.append("[GAME] [Strike Targets: ");
					for (auto& StrikeTarget : G_gameContext.ShootCoords)
					{
						StrikeCoordsSTRPRintMSG.append(1, '|');
						StrikeCoordsSTRPRintMSG.append(GetStringFromPos(StrikeTarget));
						StrikeCoordsSTRPRintMSG.append(1, '|');
						StrikeCoordsSTRPRintMSG.append(1, '\n');
					}
					StrikeCoordsSTRPRintMSG.append("]\n");
					std::cout << StrikeCoordsSTRPRintMSG;
				}
#endif
				if (G_gameContext.LastFireShotID == -1)
					G_gameContext.LastFireShotID = 0;
				else
					G_gameContext.LastFireShotID = (G_gameContext.LastFireShotID + 1) % G_gameContext.FireShotsHistroy.size();

				auto strikeCoord = G_gameContext.ShootCoords[G_gameContext.ShootCoords.size() - 1];
				G_gameContext.ShootCoords.pop_back();
#if(TEST_MACHINERY == 1)
				WriteMSGToCommandBuff(GetStringFromPos(strikeCoord) + '\n', GAME);
				std::cout << "[GAME]: " << GetStringFromPos(strikeCoord) << '\n';
#else
				std::cout << GetStringFromPos(strikeCoord) << '\n';
#endif
			};

			for (auto& StrikeResult : G_gameContext.LastThreeStrikeResults)
			{
				StrikeResult.NeedUpdate = true;
				StrikeResult.valid = false;
			};

		};
	}
}

// returns offset of next position after argument (arg list: cat[X] like) - [X] is return pos 
// modified gameArg with relevant information about current argument
int ParseGameConsoleArgument(GameConsoleMSGArgType msgType, char* buff, ParsedGameArgument& gameArg)
{

	int index = 0;
	switch (msgType)
	{
	case MSG:
	{
		char msgBuff[100];
		gameArg.isMessage = true;

		while ((*buff) != ' ' && *buff != '\0' && *buff != EOF) { msgBuff[index++] = (*buff++); };
		msgBuff[index++] = '\0';
		if (strcmp((const char*)msgBuff, "Hit!") == 0)
			gameArg.Hit = true;
		else if (strcmp((const char*)msgBuff, "Miss!") == 0)
			gameArg.Miss = true;
		else if (strcmp((const char*)msgBuff, "Win!") == 0)
			gameArg.Win = true;

		break;
	}
	case intArg:
		index = ParseConsoleArgumentINT(gameArg.valueInt, buff);
		break;
	case floatArg:
		index = ParseConsoleArgumentFLOAT(gameArg.valueFloat, buff);
		break;
	default:
		break;
	}

	return index;
}
#if(TEST_MACHINERY == 1)
//return last readed pos index
int readInput(int bufferLength, char* buffer, MSGBUFF_THREAD BUFF)
#else
int readInput(int bufferLength, char* buffer)
#endif
{
	int length = 0;
	int c;


#if(TEST_MACHINERY == 1)
	std::string MSGFromBuff = ReadMSGFromCommandBuff(BUFF);
	length += MSGFromBuff.length();
	int i = 0;
	while (i < length)
	{
		buffer[i] = MSGFromBuff[i];
		i++;
	}
	return length;
#endif

	while ((c = getchar()) != EOF && c != '\n')
	{
		if (length == bufferLength)
		{
			break;
		}

		buffer[length++] = c;
	}

	return length;
}


#if(TEST_MACHINERY == 1)
//return last readed pos index
int readInputConsole(int bufferLength, char* buffer)
{
	int length = 0;
	int c;
	while ((c = getchar()) != EOF && c != '\n')
	{
		if (length == bufferLength)
		{
			break;
		}

		buffer[length++] = c;
	}

	return length;
}
#endif

#if(TEST_MACHINERY == 1)
bool ParseGameInput(MSGBUFF_THREAD BUFF)
#else
bool ParseGameInput()
#endif
{
	static constexpr int charBuffMax = 10000;
	static char charBuff[charBuffMax];

	int length = 0;
#if(TEST_MACHINERY == 1)
	length += readInput(charBuffMax, (char*)charBuff, BUFF);
#else
	length += readInput(charBuffMax, (char*)charBuff);
#endif
	// no input
	if (!length)
		return false;

	ParsedGameArgument arg;
	int FirstArgLength = ParseGameConsoleArgument(MSG, (char*)&charBuff, arg);
	int overrallOffset = FirstArgLength;
	if (arg.isMessage)
	{
		if (arg.Win)
		{
			G_gameContext.IsGameWon = true;
			return true;
		}
		else
		{
			GameContext::StrikeResult* LatestToUpdateResult = nullptr;
			if (G_gameContext.CurrentCellMissileCheck)
				LatestToUpdateResult = &FindLastMissileCheckResult();
			else
				LatestToUpdateResult = &FindLastFireShotHistroyResult();

			auto& StrikeResult = *LatestToUpdateResult;

			StrikeResult.Hit = arg.Hit;
			if (G_gameContext.CurrentCellMissileCheck)
			{
				StrikeResult.NeedUpdate = false;
				StrikeResult.valid = true;
			}
			else
			{
				StrikeResult.valid = true;
			}

			if (arg.Miss)
			{
				ParsedGameArgument CountArg;
				overrallOffset += ParseGameConsoleArgument(intArg, ((char*)(charBuff)+overrallOffset), CountArg);
				StrikeResult.EnemiesCount = CountArg.valueInt;
				StrikeResult.EnemiesDistances.clear();
				for (int i = 0; i < CountArg.valueInt; i++)
				{
					ParsedGameArgument DistanceArg;
					overrallOffset += ParseGameConsoleArgument(floatArg, ((char*)(charBuff)+overrallOffset), DistanceArg);
					StrikeResult.EnemiesDistances.push_back(DistanceArg.valueFloat);

					if (overrallOffset > length + 1)
					{
						//std::cout << "Over Char Buff Limit while parsing game input...";
						return false;
					};

				}
			};
			G_gameContext.EnemyShipsCount -= arg.Hit;
			G_gameContext.EnemyShipsDestroyed += arg.Hit;
		}
	};

	return true;
}

int skipArgument(char* argv)
{
	if (!argv)
		return 0;

	int skipTrace = 0;
	if(argv[skipTrace] != ' ' && argv[skipTrace] != '\0')
		while ((argv[skipTrace]) != ' ' && argv[skipTrace] != '\0') skipTrace++;

	while ((argv[skipTrace]) == ' ') skipTrace++;

	return skipTrace;
}

int ParseConsoleArgumentINT(int& refValue, char* argv)
{
	if (!argv)
		return 0;
	int parsedValue = std::atoi(argv);
	refValue = parsedValue;
	return skipArgument(argv);
}

int ParseConsoleArgumentFLOAT(float& refValue, char* argv)
{
	if (!argv)
		return 0;
	float parsedValue = std::atof(argv);
	refValue = parsedValue;
	return skipArgument(argv);
}

void ParseEntryConsoleCommands(int argcs, char* argv)
{
	char* argvHead = argv;
	argvHead += ParseConsoleArgumentINT(G_gameContext.width, argvHead);
	argvHead += ParseConsoleArgumentINT(G_gameContext.heigth, argvHead);
	argvHead += ParseConsoleArgumentINT(G_gameContext.EnemyShipsCount, argvHead);
	argvHead += ParseConsoleArgumentINT(G_gameContext.MissileShipDetectionRadius, argvHead);
}