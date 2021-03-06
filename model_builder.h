#pragma once

#include "village_control.h"
#include "stair_key.h"

class Level;
class Model;
class ProgressMeter;
class Options;
struct SettlementInfo;
struct EnemyInfo;
class EnemyFactory;
class SokobanInput;
class ExternalEnemies;
struct ExternalEnemy;
class Tutorial;
class FilePath;

class ModelBuilder {
  public:
  ModelBuilder(ProgressMeter*, RandomGen&, Options*, SokobanInput*);
  PModel singleMapModel(const string& worldName);
  PModel campaignBaseModel(const string& siteName, bool externalEnemies);
  PModel campaignSiteModel(const string& siteName, EnemyId, VillainType);
  PModel tutorialModel(const string& siteName);

  void measureModelGen(const std::string& name, int numTries, function<void()> genFun);
  void measureSiteGen(int numTries, vector<std::string> types);

  PModel splashModel(const FilePath& splashPath);

  WCollective spawnKeeper(WModel, PCreature);

  static int getPigstyPopulationIncrease();
  static int getStatuePopulationIncrease();
  static int getThronePopulationIncrease();

  ~ModelBuilder();

  private:
  PModel trySingleMapModel(const string& worldName);
  PModel tryCampaignBaseModel(const string& siteName, bool externalEnemies);
  PModel tryTutorialModel(const string& siteName);
  PModel tryCampaignSiteModel(const string& siteName, EnemyId, VillainType);
  PModel tryModel(int width, const string& levelName, vector<EnemyInfo>,
      bool keeperSpawn, BiomeId, vector<ExternalEnemy>, bool wildlife);
  SettlementInfo& makeExtraLevel(WModel, EnemyInfo&);
  PModel tryBuilding(int numTries, function<PModel()> buildFun);
  void addMapVillains(vector<EnemyInfo>&, BiomeId);
  RandomGen& random;
  ProgressMeter* meter;
  Options* options;
  HeapAllocated<EnemyFactory> enemyFactory;
  SokobanInput* sokobanInput;
};
