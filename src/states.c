#include "entity.h"
#include "targeting.h"
#include "actor.h"
#include "render.h"
#include "targeting.h"
#include "movement.h"
#include "comparisons.h"
#include "tiles.h"
#include "states.h"
#include "ghostStates.h"
#include <SDL2/SDL_mixer.h>
#include "sounds.h"
#include <stdio.h>
#include "resources.h"
#include "math.h"
#include "globalData.h"


uint8_t g_current_scatter_chase_period = 0;
uint32_t g_scatter_chase_period_seconds[ NUM_SCATTER_CHASE_PERIODS ] = { 0 };


GhostMode g_current_ghost_mode = MODE_SCATTER;


// void vulnerable_enter_all( Entities *entities ) {
//     uint8_t vulnerable_animation_id = 3;
//     for( int i = 1; i < 5; ++i ) {
//         render_textures[ i ]->animation_id = vulnerable_animation_id;
//         ghosts[ i ]->direction = opposite_directions[ ghosts[ i ]->direction ];
//         ghosts[ i ]->next_tile = ghosts[ i ]->current_tile;
//     } 
// }

void vulnerable_enter( Entities *entities, EntityId ghostId ) {
    uint8_t vulnerable_texture_atlas_id = 3;
    entities->animatedSprites[ ghostId ]->texture_atlas_id = vulnerable_texture_atlas_id;
    entities->actors[ ghostId ]->direction = opposite_directions[ entities->actors[ ghostId ]->direction ];
    entities->actors[ ghostId ]->next_tile = entities->actors[ ghostId ]->current_tile;
    entities->actors[ ghostId ]->speed_multp = 0.4f;

}


void set_random_direction_and_next_tile( Entities *entities, EntityId eid, TileMap *tm ) {
    SDL_Point tile_above, tile_below, tile_left, tile_right;
    tile_above.x = entities->actors[ eid ]->current_tile.x;
    tile_above.y = entities->actors[ eid ]->current_tile.y - 1;
    tile_below.x = entities->actors[ eid ]->current_tile.x;
    tile_below.y = entities->actors[ eid ]->current_tile.y + 1;
    tile_left.x  = entities->actors[ eid ]->current_tile.x - 1;
    tile_left.y  = entities->actors[ eid ]->current_tile.y;
    tile_right.x = entities->actors[ eid ]->current_tile.x + 1;
    tile_right.y = entities->actors[ eid ]->current_tile.y;

    SDL_Point surrounding_tiles[ 4 ];
    surrounding_tiles[ DIR_UP ] = tile_above;
    surrounding_tiles[ DIR_DOWN ] = tile_below;
    surrounding_tiles[ DIR_LEFT ] = tile_left;
    surrounding_tiles[ DIR_RIGHT ] = tile_right;

    // wrap around tilemaps Issue 150
    for( int i = 0; i < 4; i++ ) {
        if( surrounding_tiles[ i ].x < 0 ) {
            surrounding_tiles[ i ].x = TILE_COLS-1;
        }
        else if( surrounding_tiles[ i ].x > TILE_COLS-1 ) {
            surrounding_tiles[ i ].x = 0;
        }
        else if( surrounding_tiles[ i ].y < 0 ) {
            surrounding_tiles[ i ].y = TILE_ROWS-1;
        }
        else if( surrounding_tiles[ i ].y > TILE_ROWS-1) {
            surrounding_tiles[ i ].y = 0;
        }
    }


    Direction randomDirection = rand() % 4; // 4 directions besides DIR_NONE

    // just choosing first open tile ghost sees
    Direction direction_to_go = opposite_directions[ entities->actors[ eid ]->direction ]; // this will ensure, that if all options are run through and ghost hasnt found a tile NOT behind him, he/she will just turn around

    // handle situation if random tile chosen is a wall
    
   
    for( int i = 0; i < 4; ++i ) {
        if( tm->tm_walls[ surrounding_tiles[ i ].y ][ surrounding_tiles[ i ].x ] == 'x' ) { 
            continue;
        }

        if( 
            points_equal(surrounding_tiles[ i ], tm->one_way_tile) 
            && i == DIR_DOWN) {
                continue;
        }
        
        if( i == opposite_directions[ entities->actors[ eid ]->direction  ] ) continue;

        direction_to_go = (Direction) i;

        break;

    }
    

    if( tm->tm_walls[ surrounding_tiles[randomDirection].y ][ surrounding_tiles[randomDirection].x ] != 'x') {
        if( 
            !points_equal(surrounding_tiles[ randomDirection ], tm->one_way_tile) 
            && randomDirection != DIR_DOWN) {
                if( randomDirection != opposite_directions[ entities->actors[ eid ]->direction  ] ) {
                    direction_to_go = randomDirection;
                }
            }
        
    }
     
    entities->actors[ eid ]->direction = direction_to_go;
    entities->actors[ eid ]->next_tile.x = surrounding_tiles[ direction_to_go ].x;
    entities->actors[ eid ]->next_tile.y = surrounding_tiles[ direction_to_go ].y;

    // 23 is the actual tilemap on the screen height - because of offset

    if( entities->actors[ eid ]->next_tile.y > TILE_ROWS-1 ) {
        entities->actors[ eid ]->next_tile.y = 1;
    } 
    if( entities->actors[ eid ]->next_tile.y < 0 ){
        entities->actors[ eid ]->next_tile.y = TILE_ROWS-1;
    }
    if( entities->actors[ eid ]->next_tile.x > TILE_COLS-1 ) {
        entities->actors[ eid ]->next_tile.x = 1;
    }
    if( entities->actors[ eid ]->next_tile.x < 0 ) {
        entities->actors[ eid ]->next_tile.x = TILE_COLS-1;
    }

}

void vulnerable_process( Entities *entities, EntityId ghostId, TileMap *tilemap ) {
    if( !Mix_Playing( GHOST_VULN_CHANNEL ) ) {
        Mix_PlayChannel( GHOST_VULN_CHANNEL, g_GhostVulnerableSound, 0 );
    }
    if( points_equal( entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile ) ) {
        set_random_direction_and_next_tile( entities, ghostId, tilemap );    
    }
}

void normal_enter( Entities *entities, EntityId ghostId )  {
    // set texture atlas id to the id
    entities->animatedSprites[ ghostId ] ->num_frames_col = 8;
    entities->animatedSprites[ ghostId ]->current_anim_row = 4;
    entities->animatedSprites[ ghostId ]->accumulator = 0.0f;
    entities->animatedSprites[ ghostId ]->texture_atlas_id = entities->animatedSprites[ ghostId ]->default_texture_atlas_id;
    entities->actors[ ghostId ]->next_tile =  entities->actors[ ghostId ]->current_tile;
    entities->actors[ ghostId ]->speed_multp = 0.8f;

}


void normal_process( Entities *entities, EntityId ghostId, EntityId *playerIds, unsigned int numPlayers, TileMap *tilemap, LevelConfig *levelConfig ){
    Actor *ghostActor = entities->actors[ ghostId ];
    //TargetingBehavior *ghostTargetBehavior = entities->targetingBehaviors[ ghostId ];

    // play sound
    if( !Mix_Playing( GHOST_SOUND_CHANNEL ) ) {
        Mix_PlayChannel( GHOST_SOUND_CHANNEL, g_GhostSound, -1 );
    }
    //SDL_Point home_tiles[ 5 ] = { shadow_home_tile, shadow_home_tile, ambush_home_tile, moody_home_tile, pokey_home_tile };
         
    // probably won't really help, but can do a lookup in hash table to get this out of a O(n^2)
    // may be useful if more ghosts or slow tiles are added in some circumstances
    entities->actors[ ghostId ]->speed_multp = 0.85f;
    for( int j = 0; j < MAX_SLOW_TILES; j++ ) {
        if ( points_equal( entities->actors[ ghostId ]->current_tile, tilemap->tm_slow_tiles[ j ] ) ) {
            entities->actors[ ghostId ]->speed_multp = 0.5f;
            break;
        }
    }
    SDL_Point homeTile;

    switch( g_current_ghost_mode ) {
        case MODE_CHASE:
            switch ( *entities->targetingBehaviors[ ghostId ] ) {
                case SHADOW_BEHAVIOR:
                    if(points_equal(  entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile )) {
                        set_shadow_target_tile( entities, ghostId, playerIds, numPlayers );
                        set_direction_and_next_tile_shortest_to_target( ghostActor, tilemap, STATE_NORMAL );
                    }
                    break;
                case AMBUSH_BEHAVIOR:
                    if( points_equal( entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile ) ) {
                        set_ambush_target_tile( entities, ghostId, playerIds, numPlayers );
                        set_direction_and_next_tile_shortest_to_target( ghostActor, tilemap, STATE_NORMAL );
                    }
                    break;
                case MOODY_BEHAVIOR:
                    if(points_equal( entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile ) ) {
                        set_moody_target_tile(  entities, ghostId, playerIds, numPlayers);
                        set_direction_and_next_tile_shortest_to_target( ghostActor, tilemap, STATE_NORMAL );
                    }
                    break;
                case POKEY_BEHAVIOR:
                    if( points_equal( entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile )) {
                        set_pokey_target_tile(  entities, ghostId, playerIds, numPlayers );
                        set_direction_and_next_tile_shortest_to_target( ghostActor, tilemap, STATE_NORMAL);
                    }
                    break;

                default:
                    fprintf( stderr, "Entity %d has invlaid targeting behavior. Behavior: %d\n", ghostId, *entities->targetingBehaviors[ ghostId ]);
                    break;
            }
            break;
        case MODE_SCATTER:
            switch( *entities->targetingBehaviors[ ghostId ] ) {
                case SHADOW_BEHAVIOR:
                    homeTile = shadow_home_tile;
                    break;
                case AMBUSH_BEHAVIOR:
                    homeTile = ambush_home_tile;
                    break;
                case MOODY_BEHAVIOR:
                    homeTile = moody_home_tile;
                    break;
                case POKEY_BEHAVIOR:
                    homeTile = pokey_home_tile;
                    break;
                
                break;
            }
            set_scatter_target_tile( entities, ghostId, homeTile);
            if( points_equal( entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile ) ) {
                set_direction_and_next_tile_shortest_to_target( entities->actors[ ghostId ], tilemap, STATE_NORMAL );
            }
    }

   // check if player should die
    EntityId playerId;
    for( int i = 0; i < numPlayers; i++ ) {
        playerId = playerIds[ i ];
        // can;t hurt
        if( *entities->invincibilityTimers[playerId] > 0.0f ) {
            continue;
        }
        if( *entities->isActive[playerId] == SDL_FALSE ) {
            continue;
        }
        if( *entities->deathTimers[playerId] > 0.0f ) {
            continue;
        }
        if ( points_equal(entities->actors[ playerId ]->current_tile, entities->actors[ ghostId ]->current_tile) ) {
            //Mix_PlayChannel(-1, g_PacDieOhNoSound, 0 );
            *entities->deathTimers[playerId] = 2.0f;
            Mix_PlayChannel(PAC_DIE_CHANNEL, g_PacDieSound, 0 );
            // close down any mirror players
            for( int eid = 0; eid < MAX_NUM_ENTITIES; eid++ ) {
                if( entities->mirrorEntityRefs[eid] == NULL ) {
                    continue;
                }
                if( *entities->mirrorEntityRefs[eid] == playerId ) {
                    *entities->activeTimers[eid] = 0;
                }
            }



            if(gLivesRemaining > 0 ){
                gLivesRemaining--;
                printf("Lives Remaining = %d\n", gLivesRemaining );
                gLivesRemainingUI.livesRemaining = gLivesRemaining;
                updateLivesRemainingTexture( &gLivesRemainingUI );
            }
            if( gLivesRemaining == 0 ) {
                *entities->isActive[playerId] = SDL_FALSE;
                printf("Player Out = %d. Eid = %d\n", i, playerId );
            }
        }
    }
    
}


// void go_to_pen_enter( Entities *entities, EntityId ghostId ){
//     //uint8_t texture_atlas_id = 4;
//     //uint8_t animation_id = 4;
//     //render_texture->animation_id = animation_id;
//     actors[ actor_id ]->next_tile = actors[ actor_id ]->current_tile;
//     actors[ actor_id ]->target_tile = ghost_pen_tile;
//     actors[ actor_id ]->speed_multp = 1.6f;
// }

/** Need to fix this!!!
 * Sometimes they can get stuck in an infinite loop.
 * I need to make sure they definitely get to the pen.
 * In the other states, its fine if they get caught and can't go to their target tile. 
 * In this state its a necessity
 */
void go_to_pen_process( Entities *entities, LevelConfig *levelConfig, EntityId ghostId, TileMap *tm ) {
    if( points_equal(entities->actors[ ghostId]->next_tile, entities->actors[ ghostId]->current_tile ) && !points_equal(entities->actors[ ghostId]->current_tile, levelConfig->ghostPenTile ) ) {
        set_direction_and_next_tile_shortest_to_target( entities->actors[ ghostId ], tm, STATE_GO_TO_PEN );
    }
}


void leave_pen_enter( Entities *entities, EntityId ghostId ) {
    entities->animatedSprites[ ghostId ] ->num_frames_col = 8;
    entities->animatedSprites[ ghostId ]->current_anim_row = 4;
    entities->animatedSprites[ ghostId ]->accumulator = 0.0f;
    entities->animatedSprites[ ghostId ]->texture_atlas_id = entities->animatedSprites[ ghostId ]->default_texture_atlas_id;
    entities->actors[ ghostId ]->next_tile =  entities->actors[ ghostId ]->current_tile;
    entities->actors[ ghostId ]->next_tile.y -= 3;
    entities->actors[ ghostId ]->speed_multp = 0.8f;

    entities->actors[ ghostId ]->target_tile.x = entities->actors[ ghostId ]->current_tile.x;
    entities->actors[ ghostId ]->target_tile.y = entities->actors[ ghostId ]->current_tile.y - 4;
    
    entities->actors[ ghostId ]->direction = DIR_UP;
}

void leave_pen_process( Entities *entities, EntityId ghostId, TileMap *tm ){
    entities->actors[ ghostId ]->speed_multp = 0.85f;
    for( int j = 0; j < MAX_SLOW_TILES; j++ ) {
        if ( points_equal( entities->actors[ ghostId ]->current_tile, tm->tm_slow_tiles[ j ] ) ) {
            entities->actors[ ghostId ]->speed_multp = 0.5f;
            break;
        }
    }
}

void stayPenEnter( Entities *entities, LevelConfig *levelConfig, EntityId ghostId ) {
    entities->animatedSprites[ ghostId ] ->num_frames_col = 8;
    entities->animatedSprites[ ghostId ]->current_anim_row = 4;
    entities->animatedSprites[ ghostId ]->accumulator = 0.0f;
    entities->animatedSprites[ ghostId ]->texture_atlas_id = entities->animatedSprites[ ghostId ]->default_texture_atlas_id;
    

    entities->actors[ ghostId ]->speed_multp = 0.8f;

    entities->actors[ ghostId ]->target_tile.x = entities->actors[ ghostId ]->current_tile.x - 1;
    entities->actors[ ghostId ]->target_tile.y = entities->actors[ ghostId ]->current_tile.y;
    
    entities->actors[ ghostId ]->direction = DIR_LEFT;
}

void stayPenProcess( Entities *entities, LevelConfig *levelConfig, TileMap *tilemap, EntityId ghostId ) {
    
    if(points_equal(  entities->actors[ ghostId ]->next_tile, entities->actors[ ghostId ]->current_tile )) {
        if( entities->actors[ghostId]->current_tile.x == levelConfig->ghostPenTile.x - 1 ) {
            entities->actors[ghostId]->target_tile.x = levelConfig->ghostPenTile.x + 1;
        }
        else if( entities->actors[ghostId]->current_tile.x == levelConfig->ghostPenTile.x + 1 ) {
            entities->actors[ghostId]->target_tile.x = levelConfig->ghostPenTile.x - 1;
        }
        set_direction_and_next_tile_shortest_to_target( entities->actors[ ghostId ], tilemap, STATE_NORMAL );
    }
}

// void states_machine_process( Actor **actors, GhostState *ghost_states, TileMap *tm ) {
    
//     // process
//     for( int i = 1; i < 5; ++i ){
//         switch( ghost_states[ i ] ) {
//             case STATE_NORMAL:
//                 normal_process( actors, i, tm );
//                 break;
//             case STATE_VULNERABLE:
//                 vulnerable_process(actors[ i ], tm );
//                 break;
//             case STATE_GO_TO_PEN:
//                 go_to_pen_process( actors[ i ], tm );
//                 break;
//             case STATE_LEAVE_PEN:
//                 leave_pen_process( actors, i, tm );
//                 break;
//         }
//     }
// }

