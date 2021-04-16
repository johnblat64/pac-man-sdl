#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "actor.h"
#include "animation.h"
#include "jb_types.h"
#include "constants.h"
#include "tiles.h"
#include "render.h"


SDL_bool g_show_debug_info = SDL_TRUE;

SDL_Color pac_color = {200,150,0};

void set_blinky_target_tile( Actor **actors, TileMap *tm ) {
    SDL_Point blinky_tile_above, blinky_tile_below, blinky_tile_left, blinky_tile_right;

    SDL_Point blinky_current_tile = actors[ 1 ]->current_tile;
    SDL_Point pac_current_tile = actors[ 0 ]->current_tile;

    blinky_tile_above.x = blinky_current_tile.x;
    blinky_tile_above.y = blinky_current_tile.y - 1;
    blinky_tile_below.x = blinky_current_tile.x;
    blinky_tile_below.y = blinky_current_tile.y + 1;
    blinky_tile_left.x = blinky_current_tile.x - 1;
    blinky_tile_left.y = blinky_current_tile.y;
    blinky_tile_right.x = blinky_current_tile.x + 1;
    blinky_tile_right.y = blinky_current_tile.y;

    float above_to_pac_dist = ( blinky_tile_above.x - pac_current_tile.x ) * ( blinky_tile_above.x - pac_current_tile.x )  
        + ( blinky_tile_above.y - pac_current_tile.y ) * ( blinky_tile_above.y - pac_current_tile.y );
    float below_to_pac_dist = ( blinky_tile_below.x - pac_current_tile.x ) * ( blinky_tile_below.x - pac_current_tile.x ) 
        + ( blinky_tile_below.y - pac_current_tile.y ) * ( blinky_tile_below.y - pac_current_tile.y );
    float left_to_pac_dist = ( blinky_tile_left.x - pac_current_tile.x ) * ( blinky_tile_left.x - pac_current_tile.x )  
        + ( blinky_tile_left.y - pac_current_tile.y ) * ( blinky_tile_left.y - pac_current_tile.y );
    float right_to_pac_dist = ( blinky_tile_right.x - pac_current_tile.x ) * ( blinky_tile_right.x - pac_current_tile.x ) 
        + ( blinky_tile_right.y - pac_current_tile.y ) * ( blinky_tile_right.y - pac_current_tile.y );

    Uint8 num_possible_tiles = 0;
    SDL_Point possible_tiles[ 3 ];
    float lengths_to_pac[ 3 ];
    //SDL_bool chosen_tile = { SDL_FALSE, SDL_FALSE, SDL_FALSE };

    if( actors[ 0 ]->direction == DIR_UP ) {
        // 
        if( tm->tm_texture_atlas_indexes[ blinky_tile_above.y ][ blinky_tile_above.x ].r == EMPTY_TILE_TEXTURE_ATLAS_INDEX.r ) {
            possible_tiles[ num_possible_tiles ] = blinky_tile_above;
            ++num_possible_tiles;
        }
        if(  tm->tm_texture_atlas_indexes[ blinky_tile_left.y ][ blinky_tile_left.x ].r == EMPTY_TILE_TEXTURE_ATLAS_INDEX.r ) {
            possible_tiles[ num_possible_tiles ] = blinky_tile_left;
            ++num_possible_tiles;
        }
        if( tm->tm_texture_atlas_indexes[ blinky_tile_right.y ][ blinky_tile_right.x ].r == EMPTY_TILE_TEXTURE_ATLAS_INDEX.r ) {
            possible_tiles[ num_possible_tiles ] = blinky_tile_right;
            ++num_possible_tiles;
        }

        // only one way to go
        if(num_possible_tiles == 1 ) {
            actors[ 1 ]->target_tile = possible_tiles[ 0 ];
        }

        // multiple choices - pick shortest one
        Uint8 index_of_largest = 0; // used for access at the end
        float length_of_largest = 0; // used for checking during iterations
        for( int i = 1; i < num_possible_tiles; ++i ) {
            lengths_to_pac[ i ] = ( possible_tiles[ i ].x - pac_current_tile.x ) * ( possible_tiles[ i ].x - pac_current_tile.x ) + ( possible_tiles[ i ].y - pac_current_tile.y ) * ( possible_tiles[ i ].y - pac_current_tile.y );
            
            if( lengths_to_pac[ i ] > length_of_largest) { 
                index_of_largest = i;
                length_of_largest = lengths_to_pac[ i ];
            }
        }

        SDL_assert( index_of_largest <= 3 );
        actors[ 1 ]->target_tile = possible_tiles[ index_of_largest ];
    }

    
}

int main( int argc, char *argv[] ) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Actor *actors[ 2 ]; //= (Actor **) malloc(sizeof(Actor *) * 5);
    Animation *animations[ 2 ];// ( Animation **) malloc(sizeof(Animation *) * 5);
    RenderTexture *render_textures[ 2 ];// = ( RenderTexture **) malloc(sizeof( RenderTexture *) * 5);
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
    
    gasted_font = TTF_OpenFont("gomarice_no_continue.ttf", 30 );
    if ( gasted_font == NULL ) {
        fprintf(stderr, "%s\n", TTF_GetError());
        exit( EXIT_FAILURE );
    }

    // INIT PACMONSTER
    Position_f initial_pos = { TILE_SIZE, TILE_SIZE * 17 };
    actors[ 0 ] = init_actor( initial_pos );
    render_textures[ 0 ] = init_render_texture( renderer, "pac_monster.png", 4);
    animations[ 0 ] = init_animation( 0, 0.08f, render_textures[ 0 ]->num_sprite_clips );

    // INIT GHOST
    Position_f ghost_pos = { TILE_SIZE, TILE_SIZE * 19};
    actors[ 1 ] = init_actor( ghost_pos );
    render_textures[ 1 ] = init_render_texture( renderer, "blinky.png", 1);
    animations[ 1 ] = init_animation( 0, 0.08f, render_textures[ 1 ]->num_sprite_clips );


    // INIT TILEMAP
    tm_init_and_load_texture( renderer, &tilemap, "maze_file" );

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
            }
        }
        if(quit) break;

        // KEYBOARD STATE

        const Uint8 *current_key_states = SDL_GetKeyboardState( NULL );

        // UPDATE PACMONSTER
        pac_try_set_direction( actors[ 0 ], current_key_states, &tilemap);
       
        pac_try_move( actors[ 0 ], &tilemap, delta_time );
       
        inc_animation_frame( animations[ 0 ], 1, delta_time);
        
        pac_collect_dot( actors[ 0 ], tilemap.tm_dots, &score, renderer );


        // ghost
        pac_try_set_direction( actors[ 1 ], current_key_states, &tilemap);
       
        pac_try_move( actors[ 1 ], &tilemap, delta_time );

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

        set_render_texture_values_based_on_actor( actors, render_textures, 2 );
        set_render_texture_values_based_on_animation( animations, render_textures, 2 );

        SDL_SetRenderDrawColor( renderer, 0,0,0,255);
        SDL_RenderClear( renderer );    

        tm_render_with_screen_position_offset( renderer, &tilemap );

        render_render_textures( renderer, render_textures, 2 );

        SDL_RenderCopy( renderer, score.score_texture, NULL, &score.score_render_dst_rect);

        // DEBUG
        if ( g_show_debug_info ) {
            SDL_SetRenderDrawColor( renderer, 150,50,50,255);
            for ( int y = 0; y < SCREEN_HEIGHT; y+= TILE_SIZE ) {
                SDL_RenderDrawLine( renderer, 0, y, SCREEN_WIDTH, y);
            }
            for ( int x = 0; x < SCREEN_WIDTH; x+= TILE_SIZE) {
                SDL_RenderDrawLine( renderer, x, 0, x, SCREEN_HEIGHT );
            }
            

            // current_tile
            for(int i = 0; i < 2; ++i) {
                SDL_SetRenderDrawColor( renderer, pac_color.r, pac_color.g, pac_color.b,150);
                SDL_Rect tile_rect = { actors[ i ]->current_tile.x * TILE_SIZE, actors[ i ]->current_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE,TILE_SIZE};
                SDL_RenderFillRect( renderer, &tile_rect);
            }
            

            // target tile 
            for( int i = 0; i < 2; ++i ) {
                SDL_SetRenderDrawColor( renderer,  pac_color.r, pac_color.g, pac_color.b, 225 );
                SDL_Rect target_rect = { actors[ i ]->target_tile.x * TILE_SIZE, actors[ i ]->target_tile.y * TILE_SIZE + tilemap.tm_screen_position.y, TILE_SIZE, TILE_SIZE };
                SDL_RenderFillRect( renderer, &target_rect );

            }
            
            // pacman center point
            SDL_SetRenderDrawColor( renderer, 255,255,255,255);
            SDL_Point points_to_draw[ 25 ];
            
            //CENTER
            points_to_draw[ 0 ].x = actors[ 0 ]->center_point.x;
            points_to_draw[ 0 ].y = actors[ 0 ]->center_point.y;
            //above
            points_to_draw[ 1 ].x = actors[ 0 ]->center_point.x;
            points_to_draw[ 1 ].y = actors[ 0 ]->center_point.y - 1;
            //below
            points_to_draw[ 2 ].x = actors[ 0 ]->center_point.x;
            points_to_draw[ 2 ].y = actors[ 0 ]->center_point.y + 1;
            //left
            points_to_draw[ 3 ].x = actors[ 0 ]->center_point.x - 1;
            points_to_draw[ 3 ].y = actors[ 0 ]->center_point.y;
            //right
            points_to_draw[ 4 ].x = actors[ 0 ]->center_point.x + 1;
            points_to_draw[ 4 ].y = actors[ 0 ]->center_point.y;
            
            // SENSORS

            // TOP SENSOR
            points_to_draw[ 5 ].x = actors[ 0 ]->top_sensor.x;
            points_to_draw[ 5 ].y = actors[ 0 ]->top_sensor.y;
            //above
            points_to_draw[ 6 ].x = actors[ 0 ]->top_sensor.x;
            points_to_draw[ 6 ].y = actors[ 0 ]->top_sensor.y - 1;
            //below
            points_to_draw[ 7 ].x = actors[ 0 ]->top_sensor.x;
            points_to_draw[ 7 ].y = actors[ 0 ]->top_sensor.y + 1;
            //left
            points_to_draw[ 8 ].x = actors[ 0 ]->top_sensor.x - 1;
            points_to_draw[ 8 ].y = actors[ 0 ]->top_sensor.y;
            //right
            points_to_draw[ 9 ].x = actors[ 0 ]->top_sensor.x + 1;
            points_to_draw[ 9 ].y = actors[ 0 ]->top_sensor.y;

            // BOTTOM SENSOR
            points_to_draw[ 10 ].x = actors[ 0 ]->bottom_sensor.x;
            points_to_draw[ 10 ].y = actors[ 0 ]->bottom_sensor.y;
            //above
            points_to_draw[ 11 ].x = actors[ 0 ]->bottom_sensor.x;
            points_to_draw[ 11 ].y = actors[ 0 ]->bottom_sensor.y - 1;
            //below
            points_to_draw[ 12 ].x = actors[ 0 ]->bottom_sensor.x;
            points_to_draw[ 12 ].y = actors[ 0 ]->bottom_sensor.y + 1;
            //left
            points_to_draw[ 13 ].x = actors[ 0 ]->bottom_sensor.x - 1;
            points_to_draw[ 13 ].y = actors[ 0 ]->bottom_sensor.y;
            //right
            points_to_draw[ 14 ].x = actors[ 0 ]->bottom_sensor.x + 1;
            points_to_draw[ 14 ].y = actors[ 0 ]->bottom_sensor.y;

            // LEFT SENSOR
            points_to_draw[ 15 ].x = actors[ 0 ]->left_sensor.x;
            points_to_draw[ 15 ].y = actors[ 0 ]->left_sensor.y;
            //above
            points_to_draw[ 16 ].x = actors[ 0 ]->left_sensor.x;
            points_to_draw[ 16 ].y = actors[ 0 ]->left_sensor.y - 1;
            //below
            points_to_draw[ 17 ].x = actors[ 0 ]->left_sensor.x;
            points_to_draw[ 17 ].y = actors[ 0 ]->left_sensor.y + 1;
            //left
            points_to_draw[ 18 ].x = actors[ 0 ]->left_sensor.x - 1;
            points_to_draw[ 18 ].y = actors[ 0 ]->left_sensor.y;
            //right
            points_to_draw[ 19 ].x = actors[ 0 ]->left_sensor.x + 1;
            points_to_draw[ 19 ].y = actors[ 0 ]->left_sensor.y;

            // RIGHT SENSOR
            points_to_draw[ 20 ].x = actors[ 0 ]->right_sensor.x;
            points_to_draw[ 20 ].y = actors[ 0 ]->right_sensor.y;
            //above
            points_to_draw[ 21 ].x = actors[ 0 ]->right_sensor.x;
            points_to_draw[ 21 ].y = actors[ 0 ]->right_sensor.y - 1;
            //below
            points_to_draw[ 22 ].x = actors[ 0 ]->right_sensor.x;
            points_to_draw[ 22 ].y = actors[ 0 ]->right_sensor.y + 1;
            //left
            points_to_draw[ 23 ].x = actors[ 0 ]->right_sensor.x - 1;
            points_to_draw[ 23 ].y = actors[ 0 ]->right_sensor.y;
            //right
            points_to_draw[ 24 ].x = actors[ 0 ]->right_sensor.x + 1;
            points_to_draw[ 24 ].y = actors[ 0 ]->right_sensor.y;



            SDL_RenderDrawPoints( renderer, points_to_draw, 25 );


        }
        SDL_RenderPresent( renderer );
    }

    // CLOSE DOWN
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_DestroyTexture( render_textures[ 0 ]->texture_atlas );
    SDL_Quit();
    
}
