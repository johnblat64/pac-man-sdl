
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "tiles.h"
#include "resources.h"
#include "jb_types.h"
#include "constants.h"

/** 
 * TILE VALUES, DIMENSIONS, ETC
 */
// const int TILE_SIZE = 40; // change to different size if necessary
// const int TILE_COLS = SCREEN_WIDTH / (TILE_SIZE );
// const int TILE_ROWS = SCREEN_HEIGHT / ( TILE_SIZE ) - 2;
// const int TOTAL_NUMBER_OF_TILES = TILE_ROWS * TILE_COLS;
// const int DOT_SIZE = 5;
// const int DOT_RADIUS = DOT_SIZE / 2;
// const int DOT_PADDING = 15;
// const float DOT_SPEED = 12;

const TwoDimensionalArrayIndex EMPTY_TILE_TEXTURE_ATLAS_INDEX = { -1, -1 };

/**
 * INITIALIZATION for tm ( tilemap )
 * 
 * @level_filename: 
 *  - NULL: will initialize an empty tilemap 
 *  - NOT NULL: will load the tilemap for the level of the filename passed
 */
void tm_init_and_load_texture( SDL_Renderer *renderer, TileMap *tm, char *level_filename ) {
    // LOAD the texture
    SDL_Surface *surface;
    surface = IMG_Load("res/tileset.png");
    tm->tm_texture_atlas = SDL_CreateTextureFromSurface( renderer, surface );
    SDL_FreeSurface( surface );

    // initialize textured tiles
    for( int row = 0; row < TILE_ROWS; ++row ) {
        for ( int col = 0; col < TILE_COLS; ++col ) {
            tm->tm_texture_atlas_indexes[ row ][ col ] = EMPTY_TILE_TEXTURE_ATLAS_INDEX;
        }
    }

    // initialize dots
    for( int row = 0; row < TILE_ROWS; row++ ){
        for( int col = 0; col < TILE_COLS; col++ ) {
            tm->tm_dots[ row ][ col ] = ' ';
        }
    }

    // initialize wall
    for( int row = 0; row < TILE_ROWS; row++ ) {
        for( int col = 0; col < TILE_COLS; col++ ) {
            tm->tm_walls[ row ][ col ] = ' ';
        }
    }

    // initialize dots normalized positions
    for( int row = 0; row < TILE_ROWS; row++ ) {
        for( int col = 0; col < TILE_COLS; col++ ) {
            tm->tm_dot_particles[ row ][ col ].position.x = ( TILE_SIZE / 2) - DOT_RADIUS;
            tm->tm_dot_particles[ row ][ col ].position.y = (  (  col + row ) ) % ( ( TILE_SIZE - DOT_SIZE - DOT_PADDING - DOT_PADDING  ) * 2 ) + DOT_PADDING;
            if ( tm->tm_dot_particles[ row ][ col ].position.y > TILE_SIZE - DOT_SIZE - DOT_PADDING ) {
                int a  = tm->tm_dot_particles[ row ][ col ].position.y - ( TILE_SIZE - DOT_SIZE - DOT_PADDING );
                tm->tm_dot_particles[ row ][ col ].position.y = ( TILE_SIZE - DOT_SIZE - DOT_PADDING ) - a;
            }
            tm->tm_dot_particles[ row ][ col ].velocity.x = 0.0f;
            tm->tm_dot_particles[ row ][ col ].velocity.y = DOT_SPEED;
        }
    }
        
    // load the texture indexes to use for each tile in the tilemap
    if( level_filename != NULL) {
        try_load_tilemap_texture_atlas_indexes_from_file( tm->tm_texture_atlas_indexes );
        try_load_dots_from_file( tm->tm_dots );
        try_load_walls_from_file( tm->tm_walls );
    }
    
    // Space for any UI elements at the top of the screen
    tm->tm_screen_position.x = 0;
    tm->tm_screen_position.y = TILE_SIZE * 2; 

    
}

/**
 * CONVERTING TILE MAP GRID COORDINATES AND SCREEN COORDINATES
 */

SDL_Point tile_grid_point_to_screen_point( SDL_Point tile_grid_point, SDL_Point tile_map_screen_position ) {
    // Thoughts. might want to just store an array of all the screen positions rather than calculate it with this value every iteration.
    // Alternatively, maybe store a bunch of SDL_Rects of the collisions for each tile
    SDL_Point screen_point;
    screen_point.y = tile_grid_point.y * TILE_SIZE + tile_map_screen_position.y;
    screen_point.x = tile_grid_point.x * TILE_SIZE + tile_map_screen_position.x;

    return screen_point;
}

SDL_Point screen_point_to_tile_grid_point( SDL_Point screen_point, SDL_Point tile_map_screen_position ) {

    SDL_Point grid_point;
    grid_point.y = ( screen_point.y - tile_map_screen_position.y ) / TILE_SIZE;
    grid_point.x = ( screen_point.x - tile_map_screen_position.x ) / TILE_SIZE;

    return grid_point; 
}

/**
 * RENDERING
 */
void tm_render_with_screen_position_offset( SDL_Renderer *renderer, TileMap *tm ) {
    // tiles
    for(int row = 0; row < TILE_ROWS; ++row ) {
        for( int col = 0; col < TILE_COLS; ++col ) {
            if( tm->tm_texture_atlas_indexes[ row ][ col ].r != -1 
                || tm->tm_texture_atlas_indexes[ row ][ col ].c != -1 ){
                
                SDL_Point render_position = { 
                    tm->tm_screen_position.x + ( TILE_SIZE * col ), 
                    tm->tm_screen_position.y + ( TILE_SIZE * row ) 
                };
                
                SDL_Point texture_position_in_atlas = { 
                    tm->tm_texture_atlas_indexes[ row ][ col ].c * TILE_SIZE, 
                    tm->tm_texture_atlas_indexes[ row ][ col ].r * TILE_SIZE  
                };
                
                SDL_Rect texture_atlas_src_rect = { 
                    texture_position_in_atlas.x, 
                    texture_position_in_atlas.y, 
                    TILE_SIZE, 
                    TILE_SIZE 
                };

                SDL_Rect render_dst_rect = {
                    render_position.x , 
                    render_position.y, 
                    TILE_SIZE, 
                    TILE_SIZE 
                };

                // Want to probably organize the structure better so that the texture atlas is closer to each 64 byte in the array
                SDL_RenderCopy( renderer, tm->tm_texture_atlas, &texture_atlas_src_rect, &render_dst_rect );
            }
        }
    }

    // dots
    for( int row = 0; row < TILE_ROWS; ++row ) {
        for( int col = 0; col < TILE_COLS; ++col ) {
            if( tm->tm_dots[ row ][ col ] == 'x' ) {

                int dot_relative_to_tile_y_position = tm->tm_dot_particles[ row ][ col ].position.y * (TILE_SIZE);
                SDL_Rect dot_rect = {
                    tm->tm_screen_position.x + ( TILE_SIZE * col ) + tm->tm_dot_particles[ row ][ col ].position.x,
                    tm->tm_screen_position.y + ( TILE_SIZE * row ) + tm->tm_dot_particles[ row ][ col ].position.y, //dot_relative_to_tile_y_position,
                    DOT_SIZE,
                    DOT_SIZE
                };
                
                SDL_SetRenderDrawColor(renderer, 200,150,0,255);
                SDL_RenderFillRect( renderer, &dot_rect );
            }
        }
    } 
}

