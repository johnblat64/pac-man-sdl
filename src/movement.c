#include <SDL2/SDL.h>
#include "entity.h"
#include "movement.h"
#include "tiles.h"
#include "comparisons.h"
#include "jb_types.h"
#include "input.h"
#include "globalData.h"

static void tile_wrap( SDL_Point *tile ) {
    if( tile->y >= TILE_ROWS ) 
        tile->y = 0;
    if( tile->y < 0 )
        tile->y = TILE_ROWS - 1;
    if( tile->x >= TILE_COLS )
        tile->x = 0;
    if( tile->x < 0 )
        tile->x = TILE_COLS - 1;
}


static void alignWorldDataBasedOnWorldPosition( Entities *entities, EntityId eid ) {

    entities->currentTiles[eid]->x = entities->worldPositions[eid]->x / TILE_SIZE;
    entities->currentTiles[eid]->y = entities->worldPositions[eid]->y / TILE_SIZE;

    if(entities->sensors[eid] != NULL){
        entities->sensors[eid]->worldTopSensor = (SDL_Point){
            entities->worldPositions[eid]->x,
            entities->worldPositions[eid]->y - (TILE_SIZE*0.5)
        };

        entities->sensors[eid]->worldBottomSensor = (SDL_Point){
            entities->worldPositions[eid]->x,
            entities->worldPositions[eid]->y + (TILE_SIZE*0.5)
        };

        entities->sensors[eid]->worldLeftSensor = (SDL_Point){
            entities->worldPositions[eid]->x - (TILE_SIZE*0.5),
            entities->worldPositions[eid]->y 
        };

        entities->sensors[eid]->worldRightSensor = (SDL_Point){
            entities->worldPositions[eid]->x + (TILE_SIZE*0.5),
            entities->worldPositions[eid]->y
        };
    }
    

    entities->collisionRects[eid]->w = TILE_SIZE*0.5;
    entities->collisionRects[eid]->h = TILE_SIZE*0.5;
    entities->collisionRects[eid]->x = entities->worldPositions[eid]->x - entities->collisionRects[eid]->w*0.5;
    entities->collisionRects[eid]->y = entities->worldPositions[eid]->y - entities->collisionRects[eid]->h*0.5;
}

static void trySetDirection( Entities *entities, EntityId entityId, TileMap *tilemap ) {
    uint8_t **inputMasks = entities->inputMasks;

    if( *inputMasks[ entityId ] & g_INPUT_UP ) {
        SDL_Point tile_above = {entities->currentTiles[entityId]->x,entities->currentTiles[entityId]->y - 1 };

        tile_wrap( &tile_above );
        

        if( tilemap->tm_walls[ tile_above.y ][ tile_above.x ] != 'x' ) 
        {
            *entities->directions[entityId] = DIR_UP;
        }
        
        else {
            return;
        }
    }

    if( *inputMasks[ entityId ] & g_INPUT_DOWN  ) {
        SDL_Point tile_below = {entities->currentTiles[entityId]->x,entities->currentTiles[entityId]->y + 1 };

        tile_wrap( &tile_below );


        if( points_equal( tile_below, tilemap->one_way_tile) ) {
            return;
        }

        if(tilemap->tm_walls[ tile_below.y ][ tile_below.x ] != 'x' ) 
        {
            *entities->directions[entityId] = DIR_DOWN;
        }
        else {
            return;
        }
    }

    if( *inputMasks[ entityId ] & g_INPUT_LEFT  ) {
        SDL_Point tile_to_left = {entities->currentTiles[entityId]->x - 1,entities->currentTiles[entityId]->y  };

        tile_wrap( &tile_to_left );

        if( points_equal( tile_to_left, tilemap->one_way_tile) ) {
            return;
        }

        if(tilemap->tm_walls[ tile_to_left.y ][ tile_to_left.x ] != 'x'  ) 
        {
            *entities->directions[entityId] = DIR_LEFT;
        }
        else {
            return;
        }
    }

    if( *inputMasks[ entityId ] & g_INPUT_RIGHT ) {
        SDL_Point tile_to_right = {entities->currentTiles[entityId]->x + 1,entities->currentTiles[entityId]->y };

        tile_wrap( &tile_to_right );

        if( points_equal( tile_to_right, tilemap->one_way_tile) ) {
            return;
        }

        if(tilemap->tm_walls[ tile_to_right.y ][ tile_to_right.x ] != 'x'  ) 
        {
            *entities->directions[entityId] = DIR_RIGHT;
        }
        else {
            return;
        }
    }
}

static void handlePlayerCollision(Entities *entities, EntityId eid ) {
    EntityId pid = 0;
    for( int pidx = 0; pidx < gNumPlayers; pidx++ ) {
        pid = gPlayerIds[pidx];
        if( pid == eid ) {
            continue;
        }
        // dont bump into inactive players
        if(entities->isActive[pid] != NULL && *entities->isActive[pid] == SDL_FALSE) {
            continue;
        }
        if( points_equal( *entities->nextTiles[pid], *entities->currentTiles[pid] ) ){
            Vector_f reversed_velocity = { -entities->velocities[pid]->x, -entities->velocities[pid]->y };
            moveActor(entities, pid, reversed_velocity );
        }
    }
}

static void alignPositionToValueIfPassed(Entities *entities, EntityId eid, float vel, float *src, float target ){
    if( vel > 0 ) {
        if( *src > target ) {
            *src = target;
            alignWorldDataBasedOnWorldPosition(entities, eid);
        }
    }
    else if( vel < 0 ) {
        if( *src < target ) {
            *src = target;
            alignWorldDataBasedOnWorldPosition(entities, eid);
        }
    }
}

static void alignPositionToCurrentTileIfSensorColliding(Entities *entities, EntityId eid, SDL_Point sensor, TileMap *tilemap){
    SDL_Point nextTileWorldPosition = tile_grid_point_to_world_point( *entities->nextTiles[eid]);
    SDL_Rect nextTileRect = { nextTileWorldPosition.x, nextTileWorldPosition.y, TILE_SIZE, TILE_SIZE  };

    if( sensor.y < nextTileRect.y + TILE_SIZE 
    && sensor.y > nextTileRect.y
    && sensor.x > nextTileRect.x 
    && sensor.x < nextTileRect.x + TILE_SIZE ) {
        // target tile is a wall
        if ( tilemap->tm_walls[ entities->nextTiles[eid]->y ][ entities->nextTiles[eid]->x ] == 'x' ) {
            Vector_f reversed_velocity = { -entities->velocities[eid]->x, -entities->velocities[eid]->y };
            moveActor(entities,eid, reversed_velocity );
        }

        // other pac-men?
        handlePlayerCollision(entities, eid );
        alignWorldDataBasedOnWorldPosition(entities, eid);
    }
}



void inputMovementSystem( Entities *entities, TileMap *tilemap, float deltaTime ) {
    uint8_t **inputMasks  = entities->inputMasks;

    for( int i = 0; i < g_NumEntities; ++i  ) {
        // check if should not process
        if( inputMasks[ i ] == NULL || entities->worldPositions[i] == NULL ) {
            continue;
        }
        if( entities->stopTimers[ i ] != NULL && *entities->stopTimers[i] > 0.0f ) {
            continue;
        }
        if( entities->deathTimers[ i ] != NULL && *entities->deathTimers[i] > 0.0f ) {
            continue;
        }
        // should process
        // don't allow changing direciton if pacman is more than half of the tile
        trySetDirection( entities, i, tilemap );

        // try moving

        if( *entities->directions[i] == DIR_UP ) {
            entities->nextTiles[i]->x = entities->currentTiles[i]->x;
            entities->nextTiles[i]->y = entities->currentTiles[i]->y - 1;
            if( entities->nextTiles[i]->y < 0 ){
                entities->nextTiles[i]->y = TILE_ROWS-1;
            } 

            // set velocity
            if ( entities->worldPositions[i]->x == tile_grid_point_to_world_point( *entities->currentTiles[i] ).x  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = 0;
                entities->velocities[i]->y = -1;
            } 
            else if( entities->worldPositions[i]->x < tile_grid_point_to_world_point( *entities->currentTiles[i] ).x  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = 0.7071068;
                entities->velocities[i]->y = -0.7071068;
            }
            else if( entities->worldPositions[i]->x >tile_grid_point_to_world_point( *entities->currentTiles[i]).x  + ( TILE_SIZE / 2 )){
            entities->velocities[i]->x = -0.7071068;
                entities->velocities[i]->y = -0.7071068;
                
            }

            entities->velocities[i]->x *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            entities->velocities[i]->y *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i]  * deltaTime;
            // move
            moveActor(entities, i, *entities->velocities[i]);
            
            // make sure pacman doesn't pass centerpt
            alignPositionToValueIfPassed(entities, i, entities->velocities[i]->x, &entities->worldPositions[i]->x, tile_grid_point_to_world_point(*entities->currentTiles[i]).x + (TILE_SIZE * 0.5 ) );

            // collision
            // top sensor is inside target tile
            alignPositionToCurrentTileIfSensorColliding(entities, i, entities->sensors[i]->worldTopSensor, tilemap );

        }

        if( *entities->directions[i] == DIR_DOWN ) {
            entities->nextTiles[i]->x = entities->currentTiles[i]->x;
            entities->nextTiles[i]->y = entities->currentTiles[i]->y + 1;
            if( entities->nextTiles[i]->y >= TILE_ROWS-1 ){
                entities->nextTiles[i]->y = 0;
            } 

            // set velocity
            if ( entities->worldPositions[i]->x == tile_grid_point_to_world_point( *entities->currentTiles[i]).x  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = 0;
                entities->velocities[i]->y = 1;
            } 
            else if( entities->worldPositions[i]->x < tile_grid_point_to_world_point( *entities->currentTiles[i] ).x  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = 0.7071068;
                entities->velocities[i]->y = 0.7071068;
            }
            else if( entities->worldPositions[i]->x >tile_grid_point_to_world_point( *entities->currentTiles[i]).x  + ( TILE_SIZE / 2 )){
                entities->velocities[i]->x = -0.7071068;
                entities->velocities[i]->y = 0.7071068;
            }
            entities->velocities[i]->x *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            entities->velocities[i]->y *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            // move
            moveActor(entities, i, *entities->velocities[i] );

            // make sure pacman doesn't pass centerpt
            alignPositionToValueIfPassed(entities, i, entities->velocities[i]->x, &entities->worldPositions[i]->x, tile_grid_point_to_world_point(*entities->currentTiles[i]).x + (TILE_SIZE * 0.5 ) );


            // collision
            alignPositionToCurrentTileIfSensorColliding(entities, i, entities->sensors[i]->worldBottomSensor, tilemap );

            SDL_Point target_tile_screen_position = tile_grid_point_to_world_point( *entities->nextTiles[i]);
            SDL_Rect target_tile_rect = { target_tile_screen_position.x, target_tile_screen_position.y, TILE_SIZE, TILE_SIZE  };

            if( entities->sensors[i]->worldBottomSensor.y > target_tile_rect.y 
            && entities->sensors[i]->worldBottomSensor.x > target_tile_rect.x 
            && entities->sensors[i]->worldBottomSensor.x < target_tile_rect.x + TILE_SIZE ) {
                // target tile is a one_way_tile
                if( points_equal(tilemap->one_way_tile, *entities->nextTiles[i])) {
                    Vector_f reversed_velocity = { -entities->velocities[i]->x, -entities->velocities[i]->y };
                    moveActor(entities,i, reversed_velocity );
                }
            }
        }

        if( *entities->directions[i] == DIR_LEFT ) {
            entities->nextTiles[i]->x = entities->currentTiles[i]->x - 1;
            entities->nextTiles[i]->y = entities->currentTiles[i]->y;
            if( entities->nextTiles[i]->x < 0 ){
                entities->nextTiles[i]->x = 0;
            } 

            // set velocity
            if ( entities->worldPositions[i]->y == tile_grid_point_to_world_point( *entities->currentTiles[i] ).y  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = -1;
                entities->velocities[i]->y = 0;
            } 
            else if( entities->worldPositions[i]->y < tile_grid_point_to_world_point( *entities->currentTiles[i] ).y  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = -0.7071068;
                entities->velocities[i]->y = 0.7071068;
            }
            else if( entities->worldPositions[i]->y >tile_grid_point_to_world_point( *entities->currentTiles[i]).y  + ( TILE_SIZE / 2 )){
                entities->velocities[i]->x = -0.7071068;
                entities->velocities[i]->y = -0.7071068;
            }

            entities->velocities[i]->x *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            entities->velocities[i]->y *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            // move
            moveActor(entities, i, *entities->velocities[i] );

            // make sure pacman doesn't pass centerpt
            alignPositionToValueIfPassed(entities, i, entities->velocities[i]->y, &entities->worldPositions[i]->y, tile_grid_point_to_world_point(*entities->currentTiles[i]).y + (TILE_SIZE * 0.5 ) );

            // collision

            alignPositionToCurrentTileIfSensorColliding(entities, i, entities->sensors[i]->worldLeftSensor, tilemap );

        }

        if( *entities->directions[i] == DIR_RIGHT ) {
            entities->nextTiles[i]->x = entities->currentTiles[i]->x + 1;
            entities->nextTiles[i]->y = entities->currentTiles[i]->y;
            if( entities->nextTiles[i]->x >= TILE_COLS ){
                entities->nextTiles[i]->x = 0;
            } 
            

            // set velocity
            if ( entities->worldPositions[i]->y == tile_grid_point_to_world_point( *entities->currentTiles[i]).y  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = 1;
                entities->velocities[i]->y = 0;
            } 
            else if( entities->worldPositions[i]->y < tile_grid_point_to_world_point( *entities->currentTiles[i] ).y  + ( TILE_SIZE / 2 ) ) {
                entities->velocities[i]->x = 0.7071068;
                entities->velocities[i]->y = 0.7071068;
            }
            else if( entities->worldPositions[i]->y >tile_grid_point_to_world_point( *entities->currentTiles[i] ).y  + ( TILE_SIZE / 2 )){
                entities->velocities[i]->x = 0.7071068;
                entities->velocities[i]->y = -0.7071068;
            }

            entities->velocities[i]->x *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            entities->velocities[i]->y *= *entities->baseSpeeds[i] * *entities->speedMultipliers[i] * deltaTime;
            // move
            moveActor(entities, i, *entities->velocities[i] );

            // make sure pacman doesn't pass centerpt
            alignPositionToValueIfPassed(entities, i, entities->velocities[i]->y, &entities->worldPositions[i]->y, tile_grid_point_to_world_point(*entities->currentTiles[i]).y + (TILE_SIZE * 0.5 ) );

            // collision
            alignPositionToCurrentTileIfSensorColliding(entities, i, entities->sensors[i]->worldRightSensor, tilemap );

        }

        // pacman animation row
        if( entities->velocities[i]->x > 0 && entities->velocities[i]->y == 0 ) { // right
            entities->animatedSprites[ i ]->current_anim_row = 0;
        }
        if( entities->velocities[i]->x < 0 && entities->velocities[i]->y == 0 ) { // left
            entities->animatedSprites[ i ]->current_anim_row = 1;
        }
        if( entities->velocities[i]->x == 0 && entities->velocities[i]->y > 0 ) { // down
            entities->animatedSprites[ i ]->current_anim_row = 2;
        }
        if( entities->velocities[i]->x == 0 && entities->velocities[i]->y < 0 ) { // up
            entities->animatedSprites[ i ]->current_anim_row = 3;
        }
        if( entities->velocities[i]->x > 0 && entities->velocities[i]->y < 0 ) { //  up-right
            entities->animatedSprites[ i ]->current_anim_row = 4;
        }
        if( entities->velocities[i]->x < 0 && entities->velocities[i]->y < 0 ) { // up-left
            entities->animatedSprites[ i ]->current_anim_row = 5;
        }
        if( entities->velocities[i]->x > 0 && entities->velocities[i]->y > 0 ) { // down-right
            entities->animatedSprites[ i ]->current_anim_row = 6;
        }
        if( entities->velocities[i]->x < 0 && entities->velocities[i]->y > 0 ) { // down-left
            entities->animatedSprites[ i ]->current_anim_row = 7;
        }
    }
    
}


static void moveActor( Entities *entities, EntityId eid, Vector_f velocity ) {
    // skip if null actor
    if( entities->worldPositions[eid] == NULL ) {
        return;
    }
    // process actor
    entities->worldPositions[eid]->x += velocity.x;
    entities->worldPositions[eid]->y += velocity.y;

    // MOVE TO OTHER SIDE OF SCREEN IF OFF EDGE
    if( entities->worldPositions[eid]->x > SCREEN_WIDTH ) {
        entities->worldPositions[eid]->x = -TILE_SIZE ;
    }
    if( entities->worldPositions[eid]->y > TILE_ROWS * TILE_SIZE  ) {
        entities->worldPositions[eid]->y = -TILE_SIZE;
    }
    if( entities->worldPositions[eid]->x < -TILE_SIZE ) {
        entities->worldPositions[eid]->x = SCREEN_WIDTH;
    }
    if( entities->worldPositions[eid]->y < -TILE_SIZE ) {        
        entities->worldPositions[eid]->y = TILE_ROWS * TILE_SIZE;
    }

    alignWorldDataBasedOnWorldPosition( entities, eid );

    
}

float g_PAC_DASH_SPEED_MULTR = 2.5f;
float g_PAC_DASH_TIME_MAX = 1.0f;
void dashTimersSystem( Entities *entities, float deltaTime ) {
    for( int eid = 0; eid < MAX_NUM_ENTITIES; eid++) {
        if( entities->dashTimers[ eid ] == NULL || entities->chargeTimers[ eid ] == NULL || entities->slowTimers[ eid ] == NULL || entities->inputMasks[ eid ] == NULL ) { //skip if doesn't have dash timers
            continue;
        }

        // charge or dash
        
        if( *entities->inputMasks[ eid ] & g_INPUT_ACTION ) {
            *entities->chargeTimers[ eid ] += deltaTime;
        }
        else if( *entities->inputMasks[ eid ] ^ g_INPUT_ACTION && *entities->chargeTimers[ eid ] > 0.0f ) {

            if( entities->dashCooldownStocks[ eid ]->currentNumStock > 0 ) {
                *entities->dashTimers[ eid ] = *entities->chargeTimers[ eid ] > g_PAC_DASH_TIME_MAX ? g_PAC_DASH_TIME_MAX : *entities->chargeTimers[ eid ];
                *entities->chargeTimers[ eid ] = 0.0f;

                // remove a stock
                entities->dashCooldownStocks[ eid ]->currentNumStock--;
                entities->dashCooldownStocks[ eid ]->cooldownTimer = entities->dashCooldownStocks[ eid ]->cooldownDuration;
            }
            else {
                *entities->chargeTimers[ eid ] = 0.0f;
            }
            

                
        }
        
        *entities->speedMultipliers[eid] = 1.0f;
        if( *entities->slowTimers[ eid ] > 0 ) {
            *entities->speedMultipliers[eid] -= 0.4f;
            *entities->slowTimers[ eid ] -= deltaTime;
        }
        if( *entities->dashTimers[eid] > 0 ) {
            *entities->speedMultipliers[eid] = g_PAC_DASH_SPEED_MULTR;
            *entities->dashTimers[ eid ] -= deltaTime;
            SDL_SetTextureAlphaMod( g_texture_atlases[ entities->animatedSprites[ eid ]->texture_atlas_id ].texture, 150 );
        }
        else {
            SDL_SetTextureAlphaMod( g_texture_atlases[ entities->animatedSprites[ eid ]->texture_atlas_id  ].texture, 255 );
        }
        if( *entities->chargeTimers[ eid ] > 0 ) {
            *entities->speedMultipliers[eid] -= 0.25f;
        }
    }
        
}


void nonPlayerInputEntityMovementSystem( Entities *entities, EntityId eid, TileMap *tm, float delta_time ) {
        Vector_f velocity = { 0, 0 };
        if( *entities->directions[eid] == DIR_UP ) {
            // set velocity
            if( entities->worldPositions[eid]->x >= tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 ) - 2 
            && ( entities->worldPositions[eid]->x <= tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 ) + 2) ) {
                entities->worldPositions[eid]->x = tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 );
            }
            if ( entities->worldPositions[eid]->x == tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 0;
                velocity.y = -1;
            } 
            else if( entities->worldPositions[eid]->x < tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 1;
                velocity.y = 0;
            }
            else if( entities->worldPositions[eid]->x > tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 )){
                velocity.x = -1;
                velocity.y = 0;
            }
        }
        else if( *entities->directions[eid] == DIR_DOWN ){
            // set velocity
            if ( entities->worldPositions[eid]->x == tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 0;
                velocity.y = 1;
                
            } 
            else if( entities->worldPositions[eid]->x < tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 1;
                velocity.y = 0;

            }
            else if( entities->worldPositions[eid]->x >tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x  + ( TILE_SIZE / 2 )){
                velocity.x = -1;
                velocity.y = 0;

            }
        } 
        else if( *entities->directions[eid] == DIR_LEFT ) {
            // set velocity
            if ( entities->worldPositions[eid]->y == tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y  + ( TILE_SIZE / 2 ) ) {
                velocity.x = -1;
                velocity.y = 0;

            } 
            else if( entities->worldPositions[eid]->y < tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 0;
                velocity.y = 1;
                
            }
            else if( entities->worldPositions[eid]->y >tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y  + ( TILE_SIZE / 2 )){
                velocity.x = 0;
                velocity.y = -1;

            }
        }
        else if(*entities->directions[eid] == DIR_RIGHT ) {
            // set velocity
            if ( entities->worldPositions[eid]->y == tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 1;
                velocity.y = 0;

            } 
            else if( entities->worldPositions[eid]->y < tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y  + ( TILE_SIZE / 2 ) ) {
                velocity.x = 0;
                velocity.y = 1;

            }
            else if( entities->worldPositions[eid]->y >tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y  + ( TILE_SIZE / 2 )){
                velocity.x = 0;
                velocity.y = -1;

            }
        }
        velocity.x *= *entities->baseSpeeds[eid] * *entities->speedMultipliers[eid] * delta_time;
        velocity.y *= *entities->baseSpeeds[eid] * *entities->speedMultipliers[eid] * delta_time;

        moveActor(entities, eid, velocity );

        // account for overshooting
        // note velocity will never be in x and y direction.
        if( velocity.x > 0 && !( *entities->directions[eid] == DIR_RIGHT )) { // right
            if( entities->worldPositions[eid]->x > tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x + ( TILE_SIZE / 2 ) ) {
                entities->worldPositions[eid]->x = ( tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x + ( TILE_SIZE / 2 ) ) ;
                alignWorldDataBasedOnWorldPosition(entities, eid );

            }
        }
        else if( velocity.x < 0 && !( *entities->directions[eid] == DIR_LEFT )) { // left
            if( entities->worldPositions[eid]->x < tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x + ( TILE_SIZE / 2 ) ) {
                entities->worldPositions[eid]->x = ( tile_grid_point_to_world_point( *entities->currentTiles[eid] ).x + ( TILE_SIZE / 2 ) ) ;
                alignWorldDataBasedOnWorldPosition(entities, eid );

            }
        }
        else if( velocity.y > 0 && !( *entities->directions[eid] == DIR_DOWN )) { // down
            if( entities->worldPositions[eid]->y > tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y + ( TILE_SIZE / 2 ) ) {
                entities->worldPositions[eid]->y = ( tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y + ( TILE_SIZE / 2 ) ) ;
                alignWorldDataBasedOnWorldPosition(entities, eid );

            }
        }
        else if( velocity.y < 0 && !( *entities->directions[eid] == DIR_UP )) { // up
            if( entities->worldPositions[eid]->y < tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y + ( TILE_SIZE / 2 ) ) {
                entities->worldPositions[eid]->y = ( tile_grid_point_to_world_point( *entities->currentTiles[eid] ).y + ( TILE_SIZE / 2 ) ) ;
                alignWorldDataBasedOnWorldPosition(entities, eid );

            }
        }
    
    
}

