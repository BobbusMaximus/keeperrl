#include "stdafx.h"
#include "model_builder.h"
#include "level.h"
#include "tribe.h"
#include "technology.h"
#include "item_type.h"
#include "inventory.h"
#include "collective_builder.h"
#include "options.h"
#include "player_control.h"
#include "spectator.h"
#include "creature.h"
#include "square.h"
#include "location.h"
#include "progress_meter.h"
#include "collective.h"
#include "name_generator.h"
#include "level_maker.h"
#include "model.h"
#include "level_builder.h"
#include "monster_ai.h"

static Location* getVillageLocation(bool markSurprise = false) {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext(), "", markSurprise);
}

typedef VillageControl::Villain VillainInfo;

enum class ExtraLevelId {
  CRYPT,
  GNOMISH_MINES,
};

struct EnemyInfo {
  SettlementInfo settlement;
  CollectiveConfig config;
  vector<VillainInfo> villains;
  optional<ExtraLevelId> extraLevel;
};


static EnemyInfo getVault(SettlementType type, CreatureFactory factory, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return {CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location(true);
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;), CollectiveConfig::noImmigrants(),
    villains};
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, villains);
}

struct FriendlyVault {
  CreatureId id;
  int min;
  int max;
};

static vector<FriendlyVault> friendlyVaults {
  {CreatureId::SPECIAL_HUMANOID, 1, 2},
  {CreatureId::ORC, 3, 8},
  {CreatureId::OGRE, 2, 5},
  {CreatureId::VAMPIRE, 2, 5},
};

static vector<EnemyInfo> getVaults(TribeSet& tribeSet) {
  vector<EnemyInfo> ret {
    getVault(SettlementType::CAVE, CreatureId::GREEN_DRAGON,
        tribeSet.killEveryone.get(), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 22}, AttackTriggerId::STOLEN_ITEMS);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);
            c.welcomeMessage = VillageControl::DRAGON_WELCOME;
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::CAVE, CreatureId::RED_DRAGON,
        tribeSet.killEveryone.get(), 1, ItemFactory::dragonCave(),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 30}, AttackTriggerId::STOLEN_ITEMS);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 12);
            c.welcomeMessage = VillageControl::DRAGON_WELCOME;
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::CAVE, CreatureId::CYCLOPS,
        tribeSet.killEveryone.get(), 1, ItemFactory::mushrooms(true),
        { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.leaderAttacks = true;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 14});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 4);
            c.attackMessage = VillageControl::CREATURE_TITLE;)}),
    getVault(SettlementType::VAULT, CreatureFactory::insects(tribeSet.monster.get()),
        tribeSet.monster.get(), Random.get(6, 12)),
    getVault(SettlementType::VAULT, CreatureId::CYCLOPS, tribeSet.monster.get(), 1,
        ItemFactory::mushrooms(true)),
    getVault(SettlementType::VAULT, CreatureId::RAT, tribeSet.pest.get(), Random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(Random.get(1, 3))) {
    FriendlyVault v = chooseRandom(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, tribeSet.keeper.get(), Random.get(v.min, v.max)));
  }
  return ret;
}

vector<EnemyInfo> getEnemyInfo(TribeSet& tribeSet) {
  vector<EnemyInfo> ret;
  for (int i : Range(Random.get(6, 12))) {
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::COTTAGE;
        c.creatures = CreatureFactory::humanVillage(tribeSet.human.get());
        c.numCreatures = Random.get(3, 7);
        c.location = new Location();
        c.tribe = tribeSet.human.get();
        c.buildingId = BuildingId::WOOD;
        c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
        CollectiveConfig::noImmigrants(), {}});
  }
  for (int i : Range(Random.get(2, 5))) {
    optional<ExtraLevelId> extraLevel;
    vector<StairKey> downStairs;
    if (i == 0) {
      extraLevel = ExtraLevelId::GNOMISH_MINES;
      downStairs = {StairKey::getNew() };
    }
    ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::SMALL_MINETOWN;
        c.creatures = CreatureFactory::gnomeVillage(tribeSet.dwarven.get());
        c.numCreatures = Random.get(3, 7);
        c.location = new Location(true);
        c.tribe = tribeSet.dwarven.get();
        c.buildingId = BuildingId::DUNGEON;
        c.downStairs = downStairs;
        c.stockpiles = LIST({StockpileInfo::MINERALS, 300});
        c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::noImmigrants(), {}, extraLevel});
  }
  ret.push_back({CONSTRUCT(SettlementInfo,
        c.type = SettlementType::ISLAND_VAULT;
        c.location = new Location(true);
        c.buildingId = BuildingId::DUNGEON;
        c.stockpiles = LIST({StockpileInfo::GOLD, 800});), CollectiveConfig::noImmigrants(), {}});
  append(ret, getVaults(tribeSet));
  append(ret, {
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::COTTAGE;
          c.creatures = CreatureFactory::singleType(tribeSet.human.get(), CreatureId::ELEMENTALIST);
          c.numCreatures = 1;
          c.location = new Location(true);
          c.tribe = tribeSet.human.get();
          c.buildingId = BuildingId::WOOD;
          c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
       CollectiveConfig::noImmigrants(),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 0;
          c.minTeamSize = 1;
          c.leaderAttacks = true;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE});
          c.behaviour = VillageBehaviour(VillageBehaviourId::CAMP_AND_SPAWN,
            CreatureFactory::elementals(tribeSet.human.get()));
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CASTLE2;
          c.creatures = CreatureFactory::vikingTown(tribeSet.human.get());
          c.numCreatures = Random.get(12, 16);
          c.location = getVillageLocation();
          c.tribe = tribeSet.human.get();
          c.buildingId = BuildingId::WOOD_CASTLE;
          c.stockpiles = LIST({StockpileInfo::GOLD, 800});
          c.guardId = CreatureId::WARRIOR;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT);
          c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
       CollectiveConfig::withImmigrants(0.003, 16, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::WARRIOR;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 6;
          c.minTeamSize = 5;
          c.triggers = LIST(
            {AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS}, AttackTriggerId::STOLEN_ITEMS);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VILLAGE;
          c.creatures = CreatureFactory::lizardTown(tribeSet.lizard.get());
          c.numCreatures = Random.get(8, 14);
          c.location = getVillageLocation();
          c.tribe = tribeSet.lizard.get();
          c.buildingId = BuildingId::MUD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT);
          c.shopFactory = ItemFactory::mushrooms();
          c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
       CollectiveConfig::withImmigrants(0.007, 15, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::LIZARDMAN;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 4;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS},
              AttackTriggerId::STOLEN_ITEMS);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VILLAGE2;
          c.creatures = CreatureFactory::elvenVillage(tribeSet.elven.get());
          c.numCreatures = Random.get(11, 18);
          c.location = getVillageLocation();
          c.tribe = tribeSet.elven.get();
          c.stockpiles = LIST({StockpileInfo::GOLD, 800});
          c.buildingId = BuildingId::WOOD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);
          c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
       CollectiveConfig::withImmigrants(0.002, 18, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::ELF_ARCHER;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
        {CONSTRUCT(VillainInfo,
          c.minPopulation = 4;
          c.minTeamSize = 4;
          c.triggers = LIST(AttackTriggerId::STOLEN_ITEMS);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::MINETOWN;
          c.creatures = CreatureFactory::dwarfTown(tribeSet.dwarven.get());
          c.numCreatures = Random.get(9, 14);
          c.location = getVillageLocation(true);
          c.tribe = tribeSet.dwarven.get();
          c.buildingId = BuildingId::DUNGEON;
          c.stockpiles = LIST({StockpileInfo::GOLD, 1000}, {StockpileInfo::MINERALS, 600});
          c.shopFactory = ItemFactory::dwarfShop();
          c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
       CollectiveConfig::withImmigrants(0.002, 15, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::DWARF;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 3;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS},
              AttackTriggerId::STOLEN_ITEMS);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 3);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CASTLE;
          c.creatures = CreatureFactory::humanCastle(tribeSet.human.get());
          c.numCreatures = Random.get(20, 26);
          c.location = getVillageLocation();
          c.tribe = tribeSet.human.get();
          c.stockpiles = LIST({StockpileInfo::GOLD, 700});
          c.buildingId = BuildingId::BRICK;
          c.guardId = CreatureId::CASTLE_GUARD;
          c.shopFactory = ItemFactory::villageShop();
          c.furniture = SquareFactory::castleFurniture(tribeSet.pest.get());),
       CollectiveConfig::withImmigrants(0.003, 26, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::KNIGHT;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::ARCHER;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),          
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 12;
          c.minTeamSize = 10;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS},
              AttackTriggerId::STOLEN_ITEMS);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::WITCH_HOUSE;
          c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::WITCH);
          c.numCreatures = 1;
          c.location = new Location();
          c.tribe = tribeSet.monster.get();
          c.buildingId = BuildingId::WOOD;
          c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);
          c.furniture = SquareFactory::single(SquareId::CAULDRON);), CollectiveConfig::noImmigrants(), {}},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CEMETERY;
          c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::ZOMBIE);
          c.numCreatures = 1;
          c.location = new Location("cemetery");
          c.tribe = tribeSet.monster.get();
          c.downStairs = { StairKey::getNew() };
          c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {}, ExtraLevelId::CRYPT},
      {CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CAVE;
          c.creatures = CreatureFactory::singleType(tribeSet.bandit.get(), CreatureId::BANDIT);
          c.numCreatures = Random.get(4, 9);
          c.location = new Location();
          c.tribe = tribeSet.bandit.get();
          c.buildingId = BuildingId::DUNGEON;),
       CollectiveConfig::withImmigrants(0.001, 10, {
           CONSTRUCT(ImmigrantInfo,
               c.id = CreatureId::BANDIT;
               c.frequency = 1;
               c.traits = LIST(MinionTrait::FIGHTER);),
           }),
       {CONSTRUCT(VillainInfo,
          c.minPopulation = 0;
          c.minTeamSize = 3;
          c.triggers = LIST({AttackTriggerId::GOLD, 500});
          c.behaviour = VillageBehaviour(VillageBehaviourId::STEAL_GOLD);
          c.attackMessage = VillageControl::TRIBE_AND_NAME;)}},
  });
  return ret;
}

int ModelBuilder::getPigstyPopulationIncrease() {
  return 4;
}

int ModelBuilder::getStatuePopulationIncrease() {
  return 1;
}

int ModelBuilder::getThronePopulationIncrease() {
  return 10;
}

static CollectiveConfig getKeeperConfig(bool fastImmigration) {
  return CollectiveConfig::keeper(
      fastImmigration ? 0.1 : 0.011,
      500,
      2,
      10,
      {
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::PIGSTY;
        c.increasePerSquare = 0.25;
        c.maxIncrease = ModelBuilder::getPigstyPopulationIncrease();),
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::STATUE;
        c.increasePerSquare = ModelBuilder::getStatuePopulationIncrease();
        c.maxIncrease = 1000;),
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::THRONE;
        c.increasePerSquare = ModelBuilder::getThronePopulationIncrease();
        c.maxIncrease = c.increasePerSquare;),
      },
      {
      CONSTRUCT(ImmigrantInfo,
        c.id = CreatureId::GOBLIN;
        c.frequency = 1;
        c.attractions = LIST(
          {{AttractionId::SQUARE, SquareId::WORKSHOP}, 1.0, 12.0},
          {{AttractionId::SQUARE, SquareId::JEWELER}, 1.0, 9.0},
          {{AttractionId::SQUARE, SquareId::FORGE}, 1.0, 9.0},
          );
        c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
        c.salary = 10;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC;
          c.frequency = 0.7;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 1.0, 12.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC_SHAMAN;
          c.frequency = 0.10;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::LIBRARY}, 1.0, 16.0},
            {{AttractionId::SQUARE, SquareId::LABORATORY}, 1.0, 9.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::OGRE;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::HARPY;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            {{AttractionId::ITEM_INDEX, ItemIndex::RANGED_WEAPON}, 1.0, 3.0, true}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_HUMANOID;
          c.frequency = 0.2;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.spawnAtDorm = true;
          c.techId = TechId::HUMANOID_MUT;
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ZOMBIE;
          c.frequency = 0.5;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 10;
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::VAMPIRE;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;
          c.spawnAtDorm = true;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 2.0, 12.0}
            );),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::LOST_SOUL;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 1.0, 9.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SUCCUBUS;
          c.frequency = 0.3;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 2.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::DOPPLEGANGER;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 4.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::RAVEN;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = SunlightState::DAY;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::BAT;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = SunlightState::NIGHT;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WOLF;
          c.frequency = 0.15;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.groupSize = Range(3, 9);
          c.autoTeam = true;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::CAVE_BEAR;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WEREWOLF;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 4.0, 12.0}
            );
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_MONSTER_KEEPER;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.spawnAtDorm = true;
          c.techId = TechId::BEAST_MUT;
          c.salary = 0;)});
}

static map<CollectiveResourceId, int> getKeeperCredit(bool resourceBonus) {
  return {{CollectiveResourceId::MANA, 200}};
  if (resourceBonus) {
    map<CollectiveResourceId, int> credit;
    for (auto elem : ENUM_ALL(CollectiveResourceId))
      credit[elem] = 10000;
    return credit;
  }
 
}

PModel ModelBuilder::collectiveModel(ProgressMeter& meter, Options* options, View* view, const string& worldName) {
  for (int i : Range(10)) {
    try {
      meter.reset();
      return tryCollectiveModel(meter, options, view, worldName);
    } catch (LevelGenException ex) {
      Debug() << "Retrying level gen";
    }
  }
  FAIL << "Couldn't generate a level";
  return nullptr;
}

static string getNewIdSuffix() {
  vector<char> chars;
  for (char c : Range(128))
    if (isalnum(c))
      chars.push_back(c);
  string ret;
  for (int i : Range(4))
    ret += chooseRandom(chars);
  return ret;
}

Level* ModelBuilder::makeExtraLevel(ProgressMeter& meter, Model* model, ExtraLevelId level, StairKey stairKey,
    Tribe* tribe) {
  switch (level) {
    case ExtraLevelId::CRYPT: 
      return model->buildLevel(
         LevelBuilder(meter, 40, 40, "Crypt"),
         LevelMaker::cryptLevel(CreatureFactory::singleType(model->tribeSet->monster.get(), CreatureId::ZOMBIE)
                .increaseLevel(5),
              SquareFactory::cryptCoffins(model->tribeSet->keeper.get()), {stairKey}, {}));
      break;
    case ExtraLevelId::GNOMISH_MINES: 
      return model->buildLevel(
         LevelBuilder(meter, 80, 60, "Gnomish mines"),
         LevelMaker::mineTownLevel(CONSTRUCT(SettlementInfo,
             c.type = SettlementType::MINETOWN;
             c.creatures = CreatureFactory::gnomeVillage(model->tribeSet->dwarven.get());
             c.numCreatures = Random.get(9, 14);
             c.location = new Location();
             c.tribe = model->tribeSet->dwarven.get();
             c.buildingId = BuildingId::DUNGEON;
             c.upStairs = {stairKey};
             c.stockpiles = LIST({StockpileInfo::GOLD, 1000}, {StockpileInfo::MINERALS, 600});
             c.shopFactory = ItemFactory::dwarfShop();
             c.furniture = SquareFactory::roomFurniture(model->tribeSet->pest.get());)));
      break;
  }
}

PModel ModelBuilder::tryCollectiveModel(ProgressMeter& meter, Options* options, View* view,
    const string& worldName) {
  Model* m = new Model(view, worldName, TribeSet());
  m->setOptions(options);
  vector<EnemyInfo> enemyInfo = getEnemyInfo(*m->tribeSet);
  vector<SettlementInfo> settlements;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    settlements.push_back(elem.settlement);
  }
  Level* top = m->buildLevel(
      LevelBuilder(meter, 250, 250, "Wilderness", false),
      LevelMaker::topLevel(CreatureFactory::forrest(m->tribeSet->wildlife.get()), settlements));
  for (auto& elem : enemyInfo)
    if (elem.extraLevel) {
      StairKey key = getOnlyElement(elem.settlement.downStairs);
      Level* level = makeExtraLevel(meter, m, *elem.extraLevel, key, elem.settlement.tribe);
      m->addLink(key, top, level);
    }
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), m->tribeSet->keeper.get())
      .setLevel(top)
      .setCredit(getKeeperCredit(options->getBoolValue(OptionId::STARTING_RESOURCE)))
      .build());
 
  m->playerCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(m->playerCollective, m, top);
  m->playerCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, m->tribeSet->keeper.get(),
      MonsterAIFactory::collective(m->playerCollective));
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  if (!keeperName.empty())
    c->setFirstName(keeperName);
  m->gameIdentifier = *c->getFirstName() + "_" + m->worldName + getNewIdSuffix();
  m->gameDisplayName = *c->getFirstName() + " of " + m->worldName;
  Creature* ref = c.get();
  top->landCreature(StairKey::keeperSpawn(), c.get());
  m->addCreature(std::move(c));
  m->playerControl->addKeeper(ref);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, m->tribeSet->keeper.get(),
        MonsterAIFactory::collective(m->playerCollective));
    top->landCreature(StairKey::keeperSpawn(), c.get());
    m->playerControl->addImp(c.get());
    m->addCreature(std::move(c));
  }
  for (int i : All(enemyInfo)) {
    if (!enemyInfo[i].settlement.collective->hasCreatures())
      continue;
    PVillageControl control;
    Location* location = enemyInfo[i].settlement.location;
    PCollective collective = enemyInfo[i].settlement.collective->addSquares(location->getAllSquares())
        .build(location->getName().get_value_or(""));
    if (!enemyInfo[i].villains.empty())
      getOnlyElement(enemyInfo[i].villains).collective = m->playerCollective;
    control.reset(new VillageControl(collective.get(), enemyInfo[i].villains));
    if (location->getName())
      m->mainVillains.push_back(collective.get());
    collective->setControl(std::move(control));
    m->collectives.push_back(std::move(collective));
  }
  return PModel(m);
}

PModel ModelBuilder::splashModel(ProgressMeter& meter, View* view, const string& splashPath) {
  Model* m = new Model(view, "", TribeSet());
  Level* l = m->buildLevel(
      LevelBuilder(meter, Level::getSplashBounds().getW(), Level::getSplashBounds().getH(), "Wilderness", false),
      LevelMaker::splashLevel(
          CreatureFactory::splashLeader(m->tribeSet->human.get()),
          CreatureFactory::splashHeroes(m->tribeSet->human.get()),
          CreatureFactory::splashMonsters(m->tribeSet->keeper.get()),
          CreatureFactory::singleType(m->tribeSet->keeper.get(), CreatureId::IMP), splashPath));
  m->spectator.reset(new Spectator(l));
  return PModel(m);
}

