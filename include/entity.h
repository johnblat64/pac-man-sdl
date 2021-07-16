#ifndef ENTITY_H
#define ENTITY_H

#include <SDL2/SDL_gamecontroller.h>
#include "render.h"
#include "actor.h"
#include "ghostStates.h"
#include "animation.h"
#include "levelConfig.h"
#include "render.h"
#include "inttypes.h"
#include "pickup.h"
#include "input.h"

#define MAX_NUM_ENTITIES 16
#define INVALID_ENTITY_ID MAX_NUM_ENTITIES
typedef unsigned int EntityId;
extern unsigned int g_NumEntities;



/**
 * If currenNumStock is less than numStockCap, then 
 * the cooldownTimer will decide when a stock is recharged
 * and can be used again
 * the stocks
 */
typedef struct CoolDownStock {
    unsigned int numStockCap;
    unsigned int currentNumStock;
    float cooldownDuration; // time until 1 stock is recharged
    float cooldownTimer; // timer for rechargine stock
} CooldownStock;




typedef struct Entities {
    SDL_bool           *isActive          [ MAX_NUM_ENTITIES ]; // process. If deactivated, can be overwritten
    Position           *positions         [ MAX_NUM_ENTITIES ];
    Actor              *actors            [ MAX_NUM_ENTITIES ]; 
    AnimatedSprite     *animatedSprites   [ MAX_NUM_ENTITIES ]; 
    RenderData         *renderDatas       [ MAX_NUM_ENTITIES ]; 
    GhostState         *ghostStates       [ MAX_NUM_ENTITIES ];
    TargetingBehavior  *targetingBehaviors[ MAX_NUM_ENTITIES ]; 
    float              *chargeTimers      [ MAX_NUM_ENTITIES ];
    float              *dashTimers        [ MAX_NUM_ENTITIES ];
    float              *slowTimers        [ MAX_NUM_ENTITIES ];
    CooldownStock      *dashCooldownStocks[ MAX_NUM_ENTITIES ];
    SDL_ScancodeToInputMask *keybinds    [ MAX_NUM_ENTITIES ]; // {      }, {    }, {        }
    uint8_t            *inputMasks        [ MAX_NUM_ENTITIES ];
    SDL_GameController *gameControllers   [ MAX_NUM_ENTITIES ];
    PickupType         *pickupTypes       [ MAX_NUM_ENTITIES ];
    unsigned int       *numDots           [ MAX_NUM_ENTITIES ];
    float              *activeTimers       [ MAX_NUM_ENTITIES ];
    unsigned int       *scores             [ MAX_NUM_ENTITIES ];
    EntityId           *mirrorEntityRefs   [ MAX_NUM_ENTITIES ];
    float              *speedBoostTimers   [ MAX_NUM_ENTITIES ];
    float              *invincibilityTimers[MAX_NUM_ENTITIES]; // won't be able to get hurt
    float              *stopTimers         [MAX_NUM_ENTITIES]; // won't move
} Entities;

EntityId createPlayer( Entities *entities, LevelConfig *levelConfig, AnimatedSprite *animatedSprite );

EntityId createGhost(  Entities *entities, LevelConfig *levelConfig, AnimatedSprite *animatedSprite, TargetingBehavior targetingBehavior );

EntityId createFruit( Entities *entities, LevelConfig *levelConfig, AnimatedSprite *animatedSprite, unsigned int numDots  );

void ghostsProcess( Entities *entities, EntityId *playerIds, unsigned int numPlayers, TileMap *tilemap, float deltaTime, LevelConfig *levelConfig );

EntityId createPowerPellet(Entities *entities, AnimatedSprite *animatedSprite, SDL_Point tile );

void collectDotProcess( Entities *entities, char dots[ TILE_ROWS ][ TILE_COLS ], unsigned int *num_dots, Score *score, SDL_Renderer *renderer );

void processTemporaryPickup( Entities *entities, EntityId *playerIds, unsigned int numPlayers, Score *score, TileMap *tilemap, unsigned int numDotsLeft, float deltaTime );

EntityId createInitialTemporaryPickup( Entities *entities, LevelConfig *levelConfig );
void processTempMirrorPlayers( Entities *entities, float deltaTime );

void tempMirrorPlayerCollectDotProcess( Entities *entities, char dots[ TILE_ROWS ][ TILE_COLS ], Score *score ) ;

void processSpeedBoostTimer( Entities *entities, float deltaTime ) ;

void overwriteSpeedBoostTimer(Entities *entities,EntityId playerId, float speed, float duration ) ;

void stopGhostsForDuration(Entities *entities, float duration);

void processStopTimers(Entities *entities, float deltaTime );

void makePlayerInvincibleForDuration( Entities *entities, EntityId playerId, float duration);
void processInvincibilityTimers( Entities *entities, float deltaTime) ;

#endif