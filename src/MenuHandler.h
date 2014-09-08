#pragma once

#include "skse/GameMenus.h"
#include "skse/GameEvents.h"
#include "skse/GameObjects.h"




struct TESTrackedStatsEvent
{
	BSFixedString	statName;
	UInt32			newValue;
};

template <>
class BSTEventSink <TESTrackedStatsEvent>
{
public:
	virtual ~BSTEventSink() {}
	virtual EventResult ReceiveEvent(TESTrackedStatsEvent * evn, EventDispatcher<TESTrackedStatsEvent> * dispatcher) = 0;
};

//Tracked Stats Event Handler
class TESTrackedStatsEventHandler : public BSTEventSink <TESTrackedStatsEvent> 
{
public:
	virtual	EventResult	ReceiveEvent(TESTrackedStatsEvent * evn, EventDispatcher<TESTrackedStatsEvent> * dispatcher);
};

extern	EventDispatcher<TESTrackedStatsEvent>*	g_trackedStatsEventDispatcher;
extern	TESTrackedStatsEventHandler				g_trackedStatsEventHandler;




//Menu Open/Close Event Handler
class LocalMenuHandler : public BSTEventSink <MenuOpenCloseEvent>
{
public:
	virtual EventResult		ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher);
};





typedef std::map <MagicItem::EffectItem*, Condition*>		ConditionedEffectMap;
typedef std::map <EnchantmentItem*, ConditionedEffectMap>	EnchantmentConditionMap;

class ConditionedWeaponEnchantments : public EnchantmentConditionMap
{
  public:
	bool Accept(EnchantmentItem* pEnch);
};


typedef ConditionedWeaponEnchantments::iterator		CndEnchantIter;
typedef ConditionedEffectMap::iterator				CndEffectIter;


class MenuCore //Statics that don't need to change between loads in the same play session
{
public:
	void InitializeMenuMonitor();

	static LocalMenuHandler					thisMenu;
	static BSFixedString					craftingMenu;
	static ConditionedWeaponEnchantments	cWeaponEnchants;
};


extern MenuCore menu;




class TrackedStatsStringHolder
{
public:
	BSFixedString	locationsDiscovered;
	BSFixedString	dungeonsCleared;
	BSFixedString	daysPassed;
	BSFixedString	hoursSlept;
	BSFixedString	hoursWaiting;
	BSFixedString	standingStonesFound;
	BSFixedString	goldFound;
	BSFixedString	mostGoldCarried;
	BSFixedString	chestsLooted;
	BSFixedString	skillIncreases;
	BSFixedString	skillBooksRead;
	BSFixedString	foodEaten;
	BSFixedString	trainingSessions;
	BSFixedString	booksRead;
	BSFixedString	horsesOwned;
	BSFixedString	housesOwned;
	BSFixedString	storesInvestedIn;
	BSFixedString	barters;
	BSFixedString	persuasions;
	BSFixedString	bribes;
	BSFixedString	intimidations;
	BSFixedString	diseasesContracted;
	BSFixedString	daysasaVampire;
	BSFixedString	daysasaWerewolf;
	BSFixedString	necksBitten;
	BSFixedString	vampirismCures;
	BSFixedString	werewolfTransformations;
	BSFixedString	mauls;
	BSFixedString	questsCompleted;
	BSFixedString	miscObjectivesCompleted;
	BSFixedString	mainQuestsCompleted;
	BSFixedString	sideQuestsCompleted;
	BSFixedString	theCompanionsQuestsCompleted;
	BSFixedString	collegeofWinterholdQuestsCompleted;
	BSFixedString	thievesGuildQuestsCompleted;
	BSFixedString	theDarkBrotherhoodQuestsCompleted;
	BSFixedString	civilWarQuestsCompleted;
	BSFixedString	daedricQuestsCompleted;
	BSFixedString	dawnguardQuestsCompleted;
	BSFixedString	dragonbornQuestsCompleted;
	BSFixedString	questlinesCompleted;
	BSFixedString	peopleKilled;
	BSFixedString	animalsKilled;
	BSFixedString	creaturesKilled;
	BSFixedString	undeadKilled;
	BSFixedString	daedraKilled;
	BSFixedString	automatonsKilled;
	BSFixedString	favoriteWeapon;
	BSFixedString	criticalStrikes;
	BSFixedString	sneakAttacks;
	BSFixedString	backstabs;
	BSFixedString	weaponsDisarmed;
	BSFixedString	brawlsWon;
	BSFixedString	bunniesSlaughtered;
	BSFixedString	spellsLearned;
	BSFixedString	favoriteSpell;
	BSFixedString	favoriteSchool;
	BSFixedString	dragonSoulsCollected;
	BSFixedString	wordsOfPowerLearned;
	BSFixedString	wordsOfPowerUnlocked;
	BSFixedString	shoutsLearned;
	BSFixedString	shoutsUnlocked;
	BSFixedString	shoutsMastered;
	BSFixedString	timesShouted;
	BSFixedString	favoriteShout;
	BSFixedString	soulGemsUsed;
	BSFixedString	soulsTrapped;
	BSFixedString	magicItemsMade;
	BSFixedString	weaponsImproved;
	BSFixedString	weaponsMade;
	BSFixedString	armorImproved;
	BSFixedString	armorMade;
	BSFixedString	potionsMixed;
	BSFixedString	potionsUsed;
	BSFixedString	poisonsMixed;
	BSFixedString	poisonsUsed;
	BSFixedString	ingredientsHarvested;
	BSFixedString	ingredientsEaten;
	BSFixedString	nirnrootsFound;
	BSFixedString	wingsPlucked;
	BSFixedString	totalLifetimeBounty;
	BSFixedString	largestBounty;
	BSFixedString	locksPicked;
	BSFixedString	pocketsPicked;
	BSFixedString	itemsPickpocketed;
	BSFixedString	timesJailed;
	BSFixedString	daysJailed;
	BSFixedString	finesPaid;
	BSFixedString	jailEscapes;
	BSFixedString	itemsStolen;
	BSFixedString	assaults;
	BSFixedString	murders;
	BSFixedString	horsesStolen;
	BSFixedString	trespasses;
	BSFixedString	eastmarchBounty;
	BSFixedString	falkreathBounty;
	BSFixedString	haafingarBounty;
	BSFixedString	hjaalmarchBounty;
	BSFixedString	thePaleBounty;
	BSFixedString	theReachBounty;
	BSFixedString	theRiftBounty;
	BSFixedString	tribalOrcsBounty;
	BSFixedString	whiterunBounty;
	BSFixedString	winterholdBounty;

	static TrackedStatsStringHolder* GetSingleton(void)
	{
		return ((TrackedStatsStringHolder*)0x012E6FB4);
	}
};
STATIC_ASSERT(sizeof(TrackedStatsStringHolder) == 0x1A0);