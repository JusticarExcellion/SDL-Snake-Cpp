/******************************************************************************
* [Title]: Snake in SDL3
* [Author]: Xander Bruce
* [Date]: 3-25-2025
*
* [Description]: Snake in SDL3 using C++
*
******************************************************************************/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmath>

//***** Basic Type Definitions *****\\

typedef uint8_t   uint8;
typedef uint8_t  ubool8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

//***** Defines & Constants *****\\

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define GRID_SIZE 32
#define UPDATE_RATE 60
#define MIN_SPAWN_DISTANCE 3

const uint16 GRID_WIDTH  = WINDOW_WIDTH  / GRID_SIZE;
const uint16 GRID_HEIGHT = WINDOW_HEIGHT / GRID_SIZE;
const uint32 MAX_BLOCKS  = GRID_WIDTH * GRID_HEIGHT;

const uint16 FPS = 5;
const uint32 SCREEN_TICKS_PER_FRAME = ( 1000 / FPS );

//***** Basic Structures *****\\

struct Block
{
	uint32 x;
	uint32 y;
	uint32 px;
	uint32 py;
} typedef Block;

struct Vector2
{
	int x;
	int y;
} typedef Vector2;

//***** Timer *****\\

struct Timer
{
	uint64 StartTicks;
	uint64 PausedTicks;
	ubool8 Started;
	ubool8 Paused;
} typedef Timer;

void
StartTimer( Timer &timer )
{
	timer.Started = true;
	timer.Paused = false;
	timer.StartTicks = SDL_GetTicks();
	timer.PausedTicks = 0;
}

void
PauseTimer( Timer &timer )
{
	if( timer.Started && !timer.Paused )
	{
		timer.Paused = true;
		timer.PausedTicks = SDL_GetTicks() - timer.StartTicks;
		timer.StartTicks = 0;
	}
}

void
UnpauseTimer( Timer &timer )
{
	if( timer.Started && timer.Paused )
	{
		timer.Paused = false;
		timer.StartTicks = SDL_GetTicks() - timer.PausedTicks;
		timer.PausedTicks = 0;
	}
}

uint64
GetTicks( Timer &timer )
{
	uint64 time = 0;
	if( timer.Started )
	{
		if( timer.Paused )
		{
			time = timer.PausedTicks;
		}
		else
		{
			time = SDL_GetTicks() - timer.StartTicks;
		}
	}

	return time;
}

Block*
GenerateNewPickup( Block* Snake, uint32 snakeCount )
{ //Creates a new pickup away from player

	const uint16 Quad_Length = GRID_WIDTH  / 2;
	const uint16 Quad_Height = GRID_HEIGHT / 2;

	Block* Pickup = ( Block* )malloc( sizeof( Block ) );
	if( Pickup == NULL )
	{
		return NULL;
	}

	Vector2 Quadrant = {};

	Quadrant.x = ( rand() % 2 );
	Quadrant.y = ( rand() % 2 );

	//NOTE: We normalize the snake's head coordinates so that we can identify which quadrant it's in
	float snakeQuad_x = std::roundf( (float)Snake[0].x / GRID_WIDTH  );
	float snakeQuad_y = std::roundf( (float)Snake[0].y / GRID_HEIGHT );

	if( ( Quadrant.x == (int)snakeQuad_x ) && ( Quadrant.y == (int)snakeQuad_y ) || ( snakeCount > 200 ) )
	{ // Flip the quadrant if we are in the same quadrant as the snake head
		Quadrant.x = ( Quadrant.x == 1 ) ? 0 : 1;
		Quadrant.y = ( Quadrant.y == 1 ) ? 0 : 1;
	}

	uint32 player_x = Snake[0].x;
	uint32 player_y = Snake[0].y;

	uint32 pickup_x = ( rand() %  Quad_Length ) + ( Quad_Length * Quadrant.x  );
	uint32 pickup_y = ( rand() %  Quad_Height ) + ( Quad_Height * Quadrant.y  );

	//TODO: If our Pickup block coordinates collides with the Snake body then we need to resample our placement point based on the following criteria
	// If we have a longer body then we need to be careful where we put the pickup point
	// The player could just be gunning forward for a variable length
	// The player could be layering their movement to resemble a looping rope
	// It could be a combination of both, at a certain length the majority of the body could lie on the other side of the "map"
	// When resampling it's okay to not worry about what quadrant the head is in

	Pickup->x = pickup_x;
	Pickup->y = pickup_y;

	Pickup->px = 0;
	Pickup->py = 0;

	return Pickup;
}

ubool8
CollisionDetection( Block* Snake, uint32 snakeSize )
{
	//Bounds Check
	if( Snake[0].x < 0 || Snake[0].x > GRID_WIDTH || Snake[0].y < 0 || Snake[0].y > GRID_HEIGHT )
	{
		return true;
	}

	//Checking for body collisions
	for( uint32 i = 1; i < snakeSize; i++ )
	{
		Block* currentBlock = &Snake[i];
		if( Snake[0].x == currentBlock->x && Snake[0].y == currentBlock->y )
		{
			return true;
		}
	}

	return false;
}

void
Restart( Block* Snake, uint32 &snakeCount )
{
	snakeCount = 1;
	Snake[0].x = GRID_WIDTH  / 2;
	Snake[0].y = GRID_HEIGHT / 2;
	Snake[0].px = 0;
	Snake[0].py = 0;
}

int
main( int argc, char **argv )
{
	//****** Init SDL ******\\

	SDL_Init( SDL_INIT_VIDEO );

	const SDL_WindowFlags WINDOW_FLAGS = 0; // Set Flags for the Window here
	const char* RENDERERING_DRIVER_NAME = NULL;

	SDL_Window* Window = SDL_CreateWindow("Snake Game", WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS );
	if( Window == NULL )
	{
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* Renderer = SDL_CreateRenderer( Window, RENDERERING_DRIVER_NAME );
	if( Renderer == NULL )
	{
		SDL_DestroyWindow( Window );
		SDL_Quit();
		return 1;
	}

	SDL_Event e;
	ubool8 quit = false;

	Block StartBlock = { ( GRID_WIDTH / 2 ) , ( GRID_HEIGHT / 2 ) , 0 , 0 };
	uint32 snakeSize = 1;
	Block Snake[MAX_BLOCKS] = {};
	Snake[0] = StartBlock;

	Vector2 direction = { 0, 1 };
	srand( NULL );

	Block* CurrentPickup = NULL;

	Timer timer = {};

	ubool8 Paused = false;


	//****** Game Loop ******\\

	while( !quit )
	{
		StartTimer( timer );
		while( SDL_PollEvent( &e ) )
		{
			switch( e.type )
			{
				case SDL_EVENT_QUIT:
					quit = true;
				break;
				case SDL_EVENT_KEY_DOWN:
					switch( e.key.key )
					{
						case SDLK_W:
							direction.x = 0;
							direction.y = -1;
						break;
						case SDLK_A:
							direction.x = -1;
							direction.y = 0;
						break;
						case SDLK_S:
							direction.x = 0;
							direction.y = 1;
						break;
						case SDLK_D:
							direction.x = 1;
							direction.y = 0;
						break;
						case SDLK_R:
							Restart( Snake, snakeSize );
						break;

						case SDLK_SPACE:
							if(! Paused )
							{
								Paused = true;
								PauseTimer( timer );
							}
							else
							{
								Paused = false;
								UnpauseTimer( timer );
							}
						break;
					}
				break;
			}
		}


		if( CurrentPickup == NULL )
		{
			CurrentPickup = GenerateNewPickup( Snake, snakeSize );
			if( CurrentPickup == NULL ) return 1; // If no pickup auto fail
		}
		else
		{
			if( Snake[0].x == CurrentPickup->x && Snake[0].y == CurrentPickup->y )
			{ // Pickup Collision Detection
				//Free Pickup
				free( CurrentPickup );
				CurrentPickup = NULL;

				//New Snake Length
				Snake[ snakeSize ] = { Snake[ snakeSize - 1].px, Snake[ snakeSize - 1].py, 0, 0 };
				++snakeSize;
			}
		}

		if( CollisionDetection( Snake,  snakeSize ) )
		{
			Restart( Snake, snakeSize );
			direction = { 0, 1};
			if( CurrentPickup != NULL )
			{
				free( CurrentPickup );
				CurrentPickup = NULL;
			}
		}

		if( !Paused )
		{

			//Clear Screen
			SDL_SetRenderDrawColor( Renderer, 0, 0, 0, 255 );
			SDL_RenderClear( Renderer );

			//Drawing the test box
			SDL_SetRenderDrawColor( Renderer, 255, 255, 255, 255 );
			for( uint32 i = 0; i < snakeSize; i++ )
			{
				Block* currentBlock = &Snake[i];
				currentBlock->px = currentBlock->x;
				currentBlock->py = currentBlock->y;

				if( i != 0 )
				{
					currentBlock->x = Snake[i - 1].px;
					currentBlock->y = Snake[i - 1].py;
				}
				else
				{
					currentBlock->x = currentBlock->x + direction.x;
					currentBlock->y = currentBlock->y + direction.y;
				}

				float x = ( currentBlock->x * (float)GRID_SIZE );
				float y = ( currentBlock->y * (float)GRID_SIZE );
				SDL_FRect tempRect = { x , y, GRID_SIZE, GRID_SIZE };
				SDL_RenderFillRect( Renderer , &tempRect );
			}

			//Render Pickup
			if( CurrentPickup != NULL )
			{
				float x = ( CurrentPickup->x * (float)GRID_SIZE );
				float y = ( CurrentPickup->y * (float)GRID_SIZE );
				SDL_FRect temp = { x, y, GRID_SIZE, GRID_SIZE  };
				SDL_RenderFillRect( Renderer , &temp );
			}

			//Present Frame
			SDL_RenderPresent( Renderer );

			//Capping FPS
			uint64 frameTicks = GetTicks( timer );
			if( frameTicks < SCREEN_TICKS_PER_FRAME )
			{
				SDL_Delay( (uint32)(SCREEN_TICKS_PER_FRAME - frameTicks) );
			}
		}
	}

	SDL_DestroyWindow( Window );
	SDL_DestroyRenderer( Renderer );
	SDL_Quit();
	return 0;
}
