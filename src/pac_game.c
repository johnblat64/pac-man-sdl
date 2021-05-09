#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "actor.h"
#include "animation.h"
#include "states.h"
#include "jb_types.h"
#include "comparisons.h"
#include "constants.h"
#include "tiles.h"
#include "render.h"


SDL_bool g_show_debug_info = SDL_TRUE;

SDL_Color pac_color = {200,150,0};

void set_cross( SDL_Point center_point, int starting_index, SDL_Point *points ) {
    points[ starting_index ].x = center_point.x;
    points[ starting_index ].y = center_point.y;
    //above
    points[ starting_index + 1 ].x = center_point.x;
    points[ starting_index + 1 ].y = center_point.y - 1;
    //below
    points[ starting_index + 2 ].x = center_point.x;
    points[ starting_index + 2 ].y = center_point.y + 1;
    //left
    points[ starting_index + 3 ].x = center_point.x - 1;
    points[ starting_index + 3 ].y = center_point.y;
    //right
    points[ starting_index + 4 ].x = center_point.x + 1;
    points[ starting_index + 4 ].y = center_point.y;
}

int main( int argc, char *argv[] ) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Actor *actors[ 5 ]; //= (Actor **) malloc(sizeof(Actor *) * 5);
    Animation *animations[ 6 ];// ( Animation **) malloc(sizeof(Animation *) * 5);
    RenderTexture *render_textures[ 6 ];// = ( RenderTexture **) malloc(sizeof( RenderTexture *) * 5);
    GhostState ghost_states[ 5 ]; // 1 thru 5
    // TIMER USED FOR VULNERABILITY STATE
    float ghost_vulnerable_timer = 0.0f;
    uint8_t ghost_vulnerable_time_seconds = 20;
    // GHOST BEHAVIOR TIMER FOR CURRENT GLOBAL GHOST MODE
    float ghost_mode_timer = 0.0f;
    float ghost_change_mode_time = g_scatter_chase_period_seconds[ g_current_scatter_chase_period ];

    
    // initialize the ghost states
    for( int i = 1; i < 5; ++i ) {
        ghost_states[ i ] = STATE_NORMAL;
    }

    TTF_Font *gasted_font; 
    TileMap tilemap;

    

    // Initializing stuff
    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf( stderr, "Error %s\n", SDL_GetError() );
        exit( EXIT_FAILURE );
    }

    if( TTF_Init() < 0 ) {
        fprintf( stderr, "%s\n", SDL_GetError() );
        exit( EXIT_FAILURE );
    }

    window = SDL_CreateWindow( "JB Pacmonster", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
    if ( window == NULL ) {
        fprintf( stderr, "Error %s\n ", SDL_GetError() );
        exit( EXIT_FAILURE );
    }

    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
    if (renderer == NULL ) {
        fprintf( stderr, "Error %s\n ", SDL_GetError() );
        exit( EXIT_FAILURE );
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (! ( IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG ) ) {
        fprintf( stderr, "Error %s\n ", IMG_GetError() );
        exit( EXIT_FAILURE );
    }
    
    gasted_font = TTF_OpenFont("res/gomarice_no_continue.ttf", 30 );
    if ( gasted_font == NULL ) {
        fprintf(stderr, "%s\n", TTF_GetError());
        exit( EXIT_FAILURE );
    }

    // INIT TEXTURE ATLASES
    add_texture_atlas( renderer, "res/pac_monster.png", 4 );
    add_texture_atlas( renderer, "res/blinky.png", 1 );
    add_texture_atlas( renderer, "res/pinky.png", 1 );
    add_texture_atlas( renderer, "res/inky.png", 1 );
    add_texture_atlas( renderer, "res/clyde.png", 1 );
    add_texture_atlas( renderer, "res/vulnerable.png", 1 );
    add_texture_atlas( renderer, "res/go_to_pen_eyes.png", 1);
    add_texture_atlas( renderer, "res/power_pellet_anim.png", 6);

    // INIT PACMONSTER
    Position_f initial_pos = { TILE_SIZE * 22, TILE_SIZE * 15 };
    actors[ 0 ] = init_actor( initial_pos );
    render_textures[ 0 ] = init_render_texture( 0 );
    animations[ 0 ] = init_animation( 0, 0.08f, g_texture_atlases[ 0 ].num_sprite_clips );
    actors[ 0 ]->speed = 160;

    // INIT GHOST
    Position_f ghost_pos = { TILE_SIZE * 22, TILE_SIZE * 11 };
    actors[ 1 ] = init_actor( ghost_pos );
    render_textures[ 1 ] = init_render_texture( 1 );
    animations[ 1 ] = init_animation( 0, 0.08f, g_texture_atlases[ 1 ].num_sprite_clips );

    Position_f pinky_position = { TILE_SIZE * 22, TILE_SIZE * 13};
    actors[ 2 ]= init_actor( pinky_position );
    render_textures[ 2 ]= init_render_texture( 2 );
    animations[ 2 ] = init_animation( 0, 0.08f, g_texture_atlases[ 2 ].num_sprite_clips );

    Position_f inky_position = { TILE_SIZE * 22, TILE_SIZE * 13};
    actors[ 3 ]= init_actor( pinky_position );
    render_textures[ 3 ]= init_render_texture( 3 );
    animations[ 3 ] = init_animation( 0, 0.08f, g_texture_atlases[ 3 ].num_sprite_clips );

    Position_f clyde_position = { TILE_SIZE * 22, TILE_SIZE * 13};
    actors[ 4 ]= init_actor( pinky_position );
    render_textures[ 4 ]= init_render_texture( 4 );
    animations[ 4 ] = init_animation( 0, 0.08f, g_texture_atlases[ 4 ].num_sprite_clips );

    for( int i = 1; i < 5; ++i ) {
        actors[ i ]->speed = 120;
    }

    // power pellet
    SDL_Point power_pellet_grid_point = { 21, 13 };
    Position_f power_pellet_position = { TILE_SIZE * 21, TILE_SIZE * 15 };
    render_textures[ 5 ] = init_render_texture( 7 );
    render_textures[ 5 ]->dest_rect.x = power_pellet_position.x; 
    render_textures[ 5 ]->dest_rect.y = power_pellet_position.y;
    render_textures[ 5 ]->dest_rect.w = TILE_SIZE; 
    render_textures[ 5 ]->dest_rect.h = TILE_SIZE;
    render_textures[ 5 ]->flip = SDL_FLIP_NONE;
    animations[ 5 ] = init_animation( 0, 0.1f, g_texture_atlases[ 7 ].num_sprite_clips );

    // INIT TILEMAP
    tm_init_and_load_texture( renderer, &tilemap, "res/maze_file" );

    SDL_Point ghost_pen_position = tile_grid_point_to_screen_point( ghost_pen_tile, tilemap.tm_screen_position ); 
    SDL_Point ghost_pen_center_point;
    ghost_pen_center_point.x = ghost_pen_position.x + (TILE_SIZE / 2);
    ghost_pen_center_point.y = ghost_pen_position.y + (TILE_SIZE / 2);

    // INIT SCORE
    Score score;
    score.font = gasted_font;
    score.score_color = pac_color;
    score.score_number = 0;
    SDL_Surface *score_surface = TTF_RenderText_Solid( score.font, "Score : 0", score.score_color);
    score.score_texture = SDL_CreateTextureFromSurface( renderer, score_surface );
    score.score_render_dst_rect.x = 10;
    score.score_render_dst_rect.y = 10;
    score.score_render_dst_rect.w = score_surface->w;
    score.score_render_dst_rect.h = score_surface->h;

    SDL_FreeSurface( score_surface );


    // PREPARE VARIABLES FOR LOOP
    SDL_Event event;
    int quit = 0;

    
    
    // delta time
    float time = 0.0;
    float max_delta_time = 1 / 60.0;
    float previous_frame_ticks = SDL_GetTicks() / 1000.0;



    while (!quit) {

        // semi-fixed timestep
        float current_frame_ticks = SDL_GetTicks() / 1000.0;
        float delta_time = current_frame_ticks - previous_frame_ticks;
        previous_frame_ticks = current_frame_ticks;
        // adjust for any pauses 
        delta_time = delta_time < max_delta_time ?  delta_time : max_delta_time;

        // EVENTS
        while (SDL_PollEvent( &event ) != 0 ) {
            if( event.type == SDL_QUIT ) {
                quit = 1;
            }
            if ( event.type == SDL_KEYDOWN ) {
                if ( event.key.keysym.sym == SDLK_b ) {
                    g_show_debug_info = !g_show_debug_info;
                }
                if (event.key.keysym.sym == SDLK_v ) {
                    ghost_vulnerable_timer = 0.0f;
                    for( int i = 1; i < 5; ++i ) {
                        ghost_states[ i ] = STATE_VULNERABLE;
                        vulnerable_enter( actors[ i ], render_textures[ i ] );
                    }
                    
                }
            }
        }
        if(quit) break;

        // KEYBOARD STATE

        const Uint8 *current_key_states = SDL_GetKeyboardState( NULL );

        // UPDATE PACMONSTER
        pac_try_set_direction( actors[ 0 ], current_key_states, &tilemap);
       
        pac_try_move( actors[ 0 ], &tilemap, delta_time );
       
        inc_animation_frame( animations[ 0 ], 1, delta_time); //pacman

        inc_animation_frame( animations[ 5 ], 1, delta_time ); // power pellet
        
        pac_collect_dot( actors[ 0 ], tilemap.tm_dots, &score, renderer );

        // ghost

        // state checks
        for(int i = 1; i < 5; ++i ) {
            switch( ghost_states[ i ] ) {
                case STATE_VULNERABLE :
                    // return all ghosts that are still vulnerable to normal
                    // when the timer runs out
                    ghost_vulnerable_timer += delta_time;
                    if (ghost_vulnerable_timer >= ghost_vulnerable_time_seconds ) {
                        for( int i = 1; i < 5; ++i ) {
                            if( ghost_states[ i ] == STATE_VULNERABLE ) {
                                ghost_states[ i ] = STATE_NORMAL;
                                normal_enter( actors[ i ], render_textures[ i ], i );
                                ghost_vulnerable_timer = 0.0f;
                            }
                        }
                    }
                    
                    // kill ghost if pacman touches
                    if ( actors[ 0 ]->current_tile.x == actors[ i ]->current_tile.x 
                    && actors[ 0 ]->current_tile.y == actors[ i ]->current_tile.y ) {
                        ghost_states[ i ] = STATE_GO_TO_PEN;
                        go_to_pen_enter( actors[ i ], render_textures[ i ], i);
                        // enter go to pen state
                    }
                    break;
                    
                case STATE_GO_TO_PEN :
                    // ghost is in pen
                    
                    if( points_equal(actors[ i ]->current_tile, ghost_pen_tile ) && actors[ i ]->center_point.y >= ghost_pen_center_point.y) {
                        actors[ i ]->direction = opposite_directions[ actors[ i ]->direction ];
                        actors[ i ]->next_tile = actors[ i ]->current_tile;
                        ghost_states[ i ] = STATE_NORMAL;
                        normal_enter( actors[ i ], render_textures[ i ], i );
                    }
                    break;

                
                case STATE_NORMAL :
                    // pacman is on power pellet
                    if ( actors[ 0 ]->current_tile.x ==  power_pellet_grid_point.x
                    && actors[ 0 ]->current_tile.y == power_pellet_grid_point.y ) {
                        ghost_states[ i ] = STATE_VULNERABLE;
                        vulnerable_enter( actors[ i ], render_textures[ i ] );
                        // enter vulnerable for this ghost
                    }
                    break;

                
            }
            
        }

        states_machine_process( actors, ghost_states, &tilemap );

        // process based on states
        // gather all ghosts in normal
        // gather all ghosts in vulnerable
        // gather all ghosts in go to pen
        // or
        // process state code for each ghost
        
        ghost_move( actors, &tilemap, delta_time );
        ghost_mode_timer += delta_time;
      
        // moving to next period
        if( g_current_scatter_chase_period < NUM_SCATTER_CHASE_PERIODS && ghost_mode_timer > g_scatter_chase_period_seconds[ g_current_scatter_chase_period ] ) {
            if( g_current_ghost_mode == MODE_CHASE ) {
                g_current_ghost_mode = MODE_SCATTER;
                for( int i = 1; i < 5; ++i ) {
                    if( ghost_states[ i ] == STATE_NORMAL ) {
                        actors[ i ]->direction = opposite_directions[ actors[ i ]->direction ];
                        actors[ i ]->next_tile = actors[ i ]->current_tile; // need to do this so that the ghost will want to set a new next tile
                    }
                    
                }
            }
            else {
                g_current_ghost_mode = MODE_CHASE;
                for( int i = 1; i < 5; ++i ) {
                    if( ghost_states[ i ] == STATE_NORMAL ) {
                        actors[ i ]->direction = opposite_directions[ actors[ i ]->direction ];
                        actors[ i ]->next_tile = actors[ i ]->current_tile; // need to do this so that the ghost will want to set a new next tile
                    }

                }
            }
            ghost_mode_timer = 0.0f;
            g_current_scatter_chase_period++;
        }

        // UPDATE DOTS ANIMATION

        for( int r = 0; r < TILE_ROWS; ++r ) {
            for( int c = 0; c < TILE_COLS; ++c ) {
                
                tilemap.tm_dot_stuff[ r ][ c ].position.y += tilemap.tm_dot_stuff[ r ][ c ].velocity.y * delta_time ;

                if ( tilemap.tm_dot_stuff[ r ][ c ].position.y < DOT_PADDING ) {
                    tilemap.tm_dot_stuff[ r ][ c ].position.y = DOT_PADDING;
                    tilemap.tm_dot_stuff[ r ][ c ].velocity.y = DOT_SPEED;
                }
                if ( tilemap.tm_dot_stuff[ r ][ c ].position.y > TILE_SIZE - DOT_SIZE - DOT_PADDING) {
                    tilemap.tm_dot_stuff[ r ][ c ].position.y = TILE_SIZE - DOT_SIZE - DOT_PADDING;
                    
                    tilemap.tm_dot_stuff[ r ][ c ].velocity.y = -DOT_SPEED;
                }
            }
        }

        // RENDER

        set_render_texture_values_based_on_actor( actors, render_textures, 5 );
        set_render_texture_values_based_on_animation( animations, render_textures, 7 );

        SDL_SetRenderDrawColor( renderer, 0,0,0,255);
        SDL_RenderClear( renderer );    

        tm_render_with_screen_position_offset( renderer, &tilemap );

        render_render_textures( renderer, render_textures, 5 );

        render_render_textures( renderer, render_textures + 5, 1);

        SDL_RenderCopy( renderer, score.score_texture, NULL, &score.score_render_dst_rect);

        // DEBUG
        if ( g_show_debug_info ) {
            //grid
            SDL_SetRenderDrawColor( renderer, 50,50,50,255);
            for ( int y = 0; y < SCREEN_HEIGHT; y+= TILE_SIZE ) {
                SDL_RenderDrawLine( renderer, 0, y, SCREEN_WIDTH, y);
            }
            for ( int x = 0; x < SCREEN_WIDTH; x+= TILE_SIZE) {
                SDL_RenderDrawLine( renderer, x, 0, x, SCREEN_HEIGHT );
            }
            
            // current_tile
            for(int i = 0; i < 4; ++i) {
                SDL_SetRenderDrawColor( renderer, pac_color.r, pac_color.g, pac_color.b,150);
                SDL_Rect tile_rect = { actors[ i ]->current_tile.x * TILE_SIZE, actors[ i ]->current_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE,TILE_SIZE};
                SDL_RenderFillRect( renderer, &tile_rect);
            }
            
            // next tile 
            for( int i = 0; i < 5; ++i ) {
                SDL_SetRenderDrawColor( renderer,  pac_color.r, pac_color.g, pac_color.b, 225 );
                SDL_Rect next_rect = { actors[ i ]->next_tile.x * TILE_SIZE, actors[ i ]->next_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE, TILE_SIZE };
                SDL_RenderFillRect( renderer, &next_rect );

            }

            SDL_SetRenderDrawColor( renderer, 255,0,0,255);
            SDL_Rect b_target_rect = { actors[ 1 ]->target_tile.x * TILE_SIZE, actors[ 1 ]->target_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderDrawRect( renderer, &b_target_rect );

            SDL_SetRenderDrawColor( renderer, 255,150,255,255);
            SDL_Rect p_target_rect = { actors[ 2 ]->target_tile.x * TILE_SIZE, actors[ 2 ]->target_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderDrawRect( renderer, &p_target_rect );

            SDL_SetRenderDrawColor( renderer, 3,252,248,255);
            SDL_Rect i_target_rect = { actors[ 3 ]->target_tile.x * TILE_SIZE, actors[ 3 ]->target_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderDrawRect( renderer, &i_target_rect );

            SDL_SetRenderDrawColor( renderer, 235, 155, 52,255);
            SDL_Rect c_target_rect = { actors[ 4 ]->target_tile.x * TILE_SIZE, actors[ 4 ]->target_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderDrawRect( renderer, &c_target_rect );
            
            // pacman center point
            SDL_SetRenderDrawColor( renderer, 255,255,255,255);
            SDL_Point points_to_draw[ 25 ];
            
            //CENTER
            set_cross( actors[ 0 ]->center_point, 0, points_to_draw );
            
            // SENSORS
            set_cross( actors[ 0 ]->top_sensor, 5, points_to_draw );
            set_cross( actors[ 0 ]->bottom_sensor, 10, points_to_draw );
            set_cross( actors[ 0 ]->left_sensor, 15, points_to_draw );
            set_cross( actors[ 0 ]->right_sensor, 20, points_to_draw );

            SDL_RenderDrawPoints( renderer, points_to_draw, 25 );

            SDL_Rect pen_rect = { ghost_pen_position.x, ghost_pen_position.y, TILE_SIZE, TILE_SIZE };
            SDL_RenderFillRect( renderer, &pen_rect);


        }
        SDL_RenderPresent( renderer );
        SDL_Delay(5);
    }

    // CLOSE DOWN
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    
    for( int i = 0; i < num_texture_atlases; ++i ) {
        SDL_DestroyTexture( g_texture_atlases[ i ].texture );
    }

    SDL_Quit();
    
}
