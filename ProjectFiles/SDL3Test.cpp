#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>

//***** Basic Type Definitions *****\\

typedef uint8_t ubool8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

//***** Screen Behavior *****\\

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

uint32
GetTicks( Timer &timer )
{
	uint32 time = 0;
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
	//TODO: We Should First Randomize the quadrant the block spawns in, then pick a point inside the quadrant, see if the point collides with the snake if it does Restart the process
	Block* Pickup = ( Block* )malloc( sizeof( Block ) );
	if( Pickup == NULL )
	{
		return NULL;
	}

	uint32 player_x = Snake[0].x;
	uint32 player_y = Snake[0].y;

	int pickup_x = player_x + ( (rand() % ( GRID_WIDTH - MIN_SPAWN_DISTANCE ) ) + MIN_SPAWN_DISTANCE - ( GRID_WIDTH / 2 ) );
	if( pickup_x < 0 ) pickup_x = 0;
	if( pickup_x > GRID_WIDTH  ) pickup_x = GRID_WIDTH;

	int pickup_y = player_y + ( (rand() % ( GRID_HEIGHT - MIN_SPAWN_DISTANCE ) ) + MIN_SPAWN_DISTANCE - ( GRID_HEIGHT / 2 ) );
	if( pickup_y < 0 ) pickup_y = 0;
	if( pickup_y > GRID_HEIGHT  ) pickup_y = GRID_HEIGHT;

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
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* Renderer = SDL_CreateRenderer( Window, RENDERERING_DRIVER_NAME );
	if( Renderer == NULL )
	{
		std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
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

			float offset = GRID_SIZE / 2.0;
			float x = ( currentBlock->x * GRID_SIZE ) - offset;
			float y = ( currentBlock->y * GRID_SIZE ) - offset;
			SDL_FRect tempRect = { x , y, GRID_SIZE, GRID_SIZE };
			SDL_RenderFillRect( Renderer , &tempRect );
		}

		//Render Pickup
		if( CurrentPickup != NULL )
		{
			float offset = GRID_SIZE / 2.0;
			float x = ( CurrentPickup->x * GRID_SIZE ) - offset;
			float y = ( CurrentPickup->y * GRID_SIZE ) - offset;
			SDL_FRect temp = { x, y, GRID_SIZE, GRID_SIZE  };
			SDL_RenderFillRect( Renderer , &temp );
		}

		//Present Frame
		SDL_RenderPresent( Renderer );

		//Capping FPS
		uint32 frameTicks = GetTicks( timer );
		if( frameTicks < SCREEN_TICKS_PER_FRAME )
		{
			SDL_Delay( SCREEN_TICKS_PER_FRAME - frameTicks );
		}
	}

	SDL_DestroyWindow( Window );
	SDL_DestroyRenderer( Renderer );
	SDL_Quit();
	return 0;
}
