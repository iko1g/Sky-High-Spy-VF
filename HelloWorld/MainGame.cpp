#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

const int displayWidth = 1280;
const int displayHeight = 720;
const int displayScale = 1;
int attachedAseteroidID;

enum PlayerState
{
	stateAppear = 0,
	stateGrounded,
	stateNotGrounded,
	stateWin,
	stateDead,
};

enum GameObjectType
{
	typeNull = -1,
	typePlayer,
	typeGem,
	typeAsteroid,
	typeAsteroidPiece,
	typeMeteor,
	typePlayerParticle,
	typeAsteroidParticle,
	typeDestroyed,
};

struct GameState
{
	int remainingGems = 3;
	int rounds = 1;
	PlayerState playerState = stateAppear; //Set to stateplay to move character
};

GameState gameState;

//--------------------------------------------------------------------
//                    UPDATE FUNCTIONS
void UpdatePlayerState();
void UpdateGems();
void UpdateAsteroidsAndPieces();
void UpdateMeteors();
void UpdatePlayerParticles();
void UpdateAsteroidParticles();
void UpdateDestroyed();
void UpdateDebugText();
//--------------------------------------------------------------------
//                    CONTROL FUNCTIONS
void HandleGroundedControls();
void HandleNotGroundedControls();
//--------------------------------------------------------------------
//                    SPAWN FUNCTIONS
void SpawnAsteroids(int);
void SpawnMeteors(int);
void SpawnPlayerOnAsteroid();
//--------------------------------------------------------------------
//                    UTILITY FUNCTIONS
void ScreenWrapping(GameObject&, bool);
float PickBetween(float, float);
bool IsOnAsteroid();
//--------------------------------------------------------------------


// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( displayWidth, displayHeight, displayScale );
	Play::CentreAllSpriteOrigins();
	Play::CreateGameObject(typePlayer, { displayWidth / 2, displayHeight / 2 }, 50, "agent8_fly");
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");
	SpawnAsteroids(gameState.remainingGems);
	SpawnMeteors(gameState.rounds);
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	Play::DrawBackground();
	UpdateGems();
	UpdateAsteroidsAndPieces();
	UpdateMeteors();
	UpdatePlayerParticles();
	UpdateAsteroidParticles();
	UpdatePlayerState();
	UpdateDestroyed();
	UpdateDebugText();
	Play::DrawFontText("64px", "REMAINING GEMS: " + std::to_string(gameState.remainingGems), { 150 , 30 }, Play::CENTRE);
	Play::DrawFontText("64px", "PRESS <- AND -> ARROW KEYS TO TURN AND SPACE TO LAUNCH", { displayWidth / 2 , displayHeight - 30 }, Play::CENTRE);
	Play::DrawFontText("64px", "ROUND " + std::to_string(gameState.rounds), { displayWidth - 50 , 30}, Play::CENTRE);
	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

//-----------------------------------------------------------------------------------------------------------------------------
//					                          UPDATE FUNCTIONS
//Update player state
void UpdatePlayerState()
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	std::vector<int> vAsteroids = Play::CollectGameObjectIDsByType(typeAsteroid);
	
	switch (gameState.playerState)
	{
	case stateAppear:
	{
		SpawnPlayerOnAsteroid();

		if (IsOnAsteroid() == true) 
		{
			gameState.playerState = stateGrounded;
		}
	}
		break;
	case stateGrounded:
	{
		//Grounded Controls
		HandleGroundedControls();
		ScreenWrapping(playerObj, true);
	}
		break;
	case stateNotGrounded:
	{
		//Not Grounded Controls
		HandleNotGroundedControls();
		ScreenWrapping(playerObj, true);
		
	}
		break;
	case stateDead:
	{
		playerObj.velocity = playerObj.velocity;
		Play::SetSprite(playerObj, "agent8_dead", 0.2f);
		playerObj.rotation = std::atan2(playerObj.velocity.width, -playerObj.velocity.height);
		ScreenWrapping(playerObj, true);
		Play::DrawFontText("64px", "PRESS ENTER TO CONTINUE", { displayWidth / 2 , displayHeight / 2}, Play::CENTRE);

		if (Play::KeyPressed(VK_RETURN) == true)
		{
			gameState.remainingGems = 3;
			gameState.rounds = 1;
			Play::StartAudioLoop("music");

			if (!Play::CollectGameObjectIDsByType(typeAsteroid).empty()) 
			{
				for (int asteroidID : Play::CollectGameObjectIDsByType(typeAsteroid))
				{
					Play::GetGameObject(asteroidID).type = typeDestroyed;
				}
			}

			if (!Play::CollectGameObjectIDsByType(typeMeteor).empty())
			{
				for (int meteorID : Play::CollectGameObjectIDsByType(typeMeteor))
				{
					Play::GetGameObject(meteorID).type = typeDestroyed;
				}
			}

			if (!Play::CollectGameObjectIDsByType(typeGem).empty())
			{
				for (int gemID : Play::CollectGameObjectIDsByType(typeGem))
				{
					Play::GetGameObject(gemID).type = typeDestroyed;
				}
			}

			if (!Play::CollectGameObjectIDsByType(typeAsteroidPiece).empty())
			{
				for (int asteroidPieceID : Play::CollectGameObjectIDsByType(typeAsteroidPiece))
				{
					Play::GetGameObject(asteroidPieceID).type = typeDestroyed;
				}
			}

			if (!Play::CollectGameObjectIDsByType(typeAsteroidParticle).empty())
			{
				for (int asteroidPieceParticleID : Play::CollectGameObjectIDsByType(typeAsteroidParticle))
				{
					Play::GetGameObject(asteroidPieceParticleID).type = typeDestroyed;
				}
			}

			SpawnAsteroids(gameState.remainingGems);
			SpawnMeteors(gameState.rounds);
			gameState.playerState = stateAppear;
		}
	}
		break;
	case stateWin:
	{
		gameState.rounds++;
		gameState.remainingGems = floor(gameState.rounds * 2.5);

		SpawnAsteroids(gameState.remainingGems);
		SpawnMeteors(gameState.rounds);
		gameState.playerState = stateAppear;
	}
		break;
	}
	Play::UpdateGameObject(playerObj);
	Play::DrawObjectRotated(playerObj);
}

//Used to update and delete gems when they are collected
void UpdateGems()
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	std::vector<int> vGems = Play::CollectGameObjectIDsByType(typeGem);
	
	for (int gemID : vGems) 
	{
		GameObject& gemObj = Play::GetGameObject(gemID);
		bool hasCollided = false;

		if (gameState.playerState == stateNotGrounded)
		{
			if (Play::IsColliding(gemObj, playerObj))
			{
				if (gameState.playerState == stateDead)
				{
					gameState.remainingGems = gameState.remainingGems;
				}
				else if (gameState.remainingGems - 1 == 0)
				{
					hasCollided = true;
					gameState.remainingGems--;
					Play::PlayAudio("reward");
					gameState.playerState = stateWin;
				}
				else
				{
					hasCollided = true;
					gameState.remainingGems--;
					Play::PlayAudio("reward");
				}
			}
		}
		Play::UpdateGameObject(gemObj);
		Play::DrawObject(gemObj);

		if (hasCollided)
			{
				Play::DestroyGameObject(gemID);          //Destroying gem object if player has collided with it
			}
		}
	
}

//Used to update asteroid and asteroid pieces when player launches off asteroid. 
void UpdateAsteroidsAndPieces()
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	std::vector<int> vAsteroids = Play::CollectGameObjectIDsByType(typeAsteroid);


	for (int asteroidID : vAsteroids)
	{
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);               //if player and asteorid is colliding
																				//update attached asteroid global variable
		if (Play::IsColliding(asteroidObj, playerObj))							//set state to grounded
		{
			attachedAseteroidID = asteroidID;
			gameState.playerState = stateGrounded;
		}
	}

	if (gameState.playerState == stateGrounded && Play::KeyPressed(VK_SPACE)) 
	{
		GameObject& attachedasteroidObj = Play::GetGameObject(attachedAseteroidID);
		Play::DrawObjectTransparent(attachedasteroidObj, 0);
		attachedasteroidObj.velocity = { 0,0 };

		int gemID = Play::CreateGameObject(typeGem, attachedasteroidObj.pos, 30 , "gem");
		GameObject& gemObj = Play::GetGameObject(gemID);

		int asteroidPieceUID = Play::CreateGameObject(typeAsteroidPiece, gemObj.pos, 10, "asteroid_pieces");
		int asteroidPieceLID = Play::CreateGameObject(typeAsteroidPiece, gemObj.pos, 10, "asteroid_pieces"); //Creating asteroid pieces gameobject IDs
		int asteroidPieceRID = Play::CreateGameObject(typeAsteroidPiece, gemObj.pos, 10, "asteroid_pieces");

		GameObject& asteroidPieceUObj = Play::GetGameObject(asteroidPieceUID);
		GameObject& asteroidPieceLObj = Play::GetGameObject(asteroidPieceLID);  //declaring gameObjects 
		GameObject& asteroidPieceRObj = Play::GetGameObject(asteroidPieceRID);

		asteroidPieceUObj.frame = 0;
		asteroidPieceLObj.frame = 1; // set frame of each 
		asteroidPieceRObj.frame = 2;

		asteroidPieceUObj.velocity = {0, -4};
		asteroidPieceLObj.velocity = {-4, 0}; //set velocities
		asteroidPieceRObj.velocity = {4, 0};
		attachedasteroidObj.type = typeDestroyed;
	}

	std::vector<int> vAsteroidPieces = Play::CollectGameObjectIDsByType(typeAsteroidPiece);

	for (int asteroidPieceID : vAsteroidPieces)         //If object is not visible on screen destroy it
	{
		GameObject& asteroidPieceObj = Play::GetGameObject(asteroidPieceID);

		Play::UpdateGameObject(asteroidPieceObj);
		Play::DrawObject(asteroidPieceObj);

		if (!Play::IsVisible(asteroidPieceObj))
		{
			asteroidPieceObj.type = typeDestroyed;
		}
	}

	for (int asteroidID : vAsteroids)
	{
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);
		ScreenWrapping(asteroidObj, true);
		Play::UpdateGameObject(asteroidObj);
		Play::DrawObjectRotated(asteroidObj);
	}
}

//Used to update asteroid 
void UpdateMeteors()
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	std::vector<int> vMeteors = Play::CollectGameObjectIDsByType(typeMeteor);

	for (int meteorID : vMeteors)
	{
		GameObject& meteorObj = Play::GetGameObject(meteorID);

		if (gameState.playerState != stateDead && Play::IsColliding(playerObj, meteorObj) && !Play::IsColliding(Play::GetGameObject(attachedAseteroidID), playerObj))
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("combust");
			gameState.playerState = stateDead;
		}
		ScreenWrapping(meteorObj, true);
		Play::UpdateGameObject(meteorObj);
		Play::DrawObjectRotated(meteorObj);
	}
}

//Used to update player particle position
void UpdatePlayerParticles()
{
	//if player is not on asteroids then
	//spawn particles while its moving maybe spawning on oldPos
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);

	if (gameState.playerState == stateNotGrounded) 
	{
		int playerParticleID = Play::CreateGameObject(typePlayerParticle, playerObj.pos, 20, "particle");
		GameObject& particleObj = Play::GetGameObject(playerParticleID);
		float playerToAsteroidAng = atan2f(particleObj.pos.y - playerObj.pos.y, particleObj.pos.x - playerObj.pos.x);
		particleObj.pos.x = playerObj.pos.x + 2 * cos(playerToAsteroidAng);                                            //Spawn Particle So it follows path of playerobj
		particleObj.pos.y = playerObj.pos.y + 2 * sin(playerToAsteroidAng);
	}

	std::vector<int> vPlayerParticles = Play::CollectGameObjectIDsByType(typePlayerParticle);

	for (int playerParticleIDs : vPlayerParticles)
	{
		GameObject& particleObj = Play::GetGameObject(playerParticleIDs);

		particleObj.scale -= 0.05;

		if (particleObj.scale <= 0.8 )
		{
			particleObj.type = typeDestroyed;
		}

		Play::UpdateGameObject(particleObj);
		Play::DrawObject(particleObj);
	}
}

//Used to update asteroid pieces particle position
void UpdateAsteroidParticles()
{
	std::vector<int> vAsteroidPieces = Play::CollectGameObjectIDsByType(typeAsteroidPiece);

	for(int asteroidPieceID : vAsteroidPieces)
	{
		GameObject& asteroidPieceObj = Play::GetGameObject(asteroidPieceID);

		if (Play::IsVisible(asteroidPieceObj)) 
		{
			int asteroidParticleID = Play::CreateGameObject(typeAsteroidParticle, asteroidPieceObj.pos, 20, "particle");
		}
	}

	std::vector<int> vAsteroidPieceParticles = Play::CollectGameObjectIDsByType(typeAsteroidParticle);

	for (int asteroidPieceParticleID : vAsteroidPieceParticles) 
	{
		GameObject& asteroidPieceParticleObj = Play::GetGameObject(asteroidPieceParticleID);

		asteroidPieceParticleObj.scale -= 0.05;

		if (asteroidPieceParticleObj.scale != 1)
		{
			asteroidPieceParticleObj.type = typeDestroyed;
		}
		Play::UpdateGameObject(asteroidPieceParticleObj);
		Play::DrawObject(asteroidPieceParticleObj);
	}
}

//Used to destroy game objects and play 'flicker' animation
void UpdateDestroyed()
{
	std::vector<int> vDess = Play::CollectGameObjectIDsByType(typeDestroyed);

	for (int desID : vDess)
	{
		GameObject& desObj = Play::GetGameObject(desID);
		desObj.animSpeed = 0.2f;
		Play::UpdateGameObject(desObj);

		if (desObj.frame % 2)
		{
			Play::DrawObjectRotated(desObj, (10 - desObj.frame) / 10.0f);
		}

		if (!Play::IsVisible(desObj) || desObj.frame >= 10)
		{
			Play::DestroyGameObject(desID);
		}
	}
}

//-----------------------------------------------------------------------------------------------------------------------------
//					                          DEBUG FUNCTIONS
//Used to check object collisions, positions and stateChanges
void UpdateDebugText()
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	std::vector<int> vAsteroids = Play::CollectGameObjectIDsByType(typeAsteroid);
	std::vector<int> vGem = Play::CollectGameObjectIDsByType(typeGem);
	std::vector<int> vMeteor = Play::CollectGameObjectIDsByType(typeMeteor);

	//COLLISION CHECKS:
	//to see if asteroid is colliding with gem or gem has same pos value as asteroid
	for (int asteroidID : vAsteroids)
	{
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);

		for (int gemID : vGem)
		{
			GameObject& gemObj = Play::GetGameObject(gemID);

			if (Play::IsColliding(asteroidObj, gemObj))
			{
				Play::DrawDebugText({ gemObj.pos.x,gemObj.pos.y }, "Gem is Touching Asteroid", Play::cGreen, true);
			}
			else if (gemObj.pos == asteroidObj.pos)
			{
				Play::DrawDebugText({ gemObj.pos.x,gemObj.pos.y }, "Gem and Asteroid Have Same Pos Value", Play::cGreen, true);
			}
		}
	}

	//to see player asteroid collision
	for (int asteroidID : vAsteroids)
	{
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);

		if (Play::IsColliding(asteroidObj, playerObj))
		{
			Play::DrawDebugText({ asteroidObj.pos.x,asteroidObj.pos.y }, "Asteroid is Colliding with player", Play::cGreen, true);
		}

	}

	//test to check for player meteor collision
	for (int meteorID : vMeteor)
	{
		GameObject& meteorObj = Play::GetGameObject(meteorID);

		if (Play::IsColliding(meteorObj, playerObj))
		{
			Play::DrawDebugText({ meteorObj.pos.x,meteorObj.pos.y }, "Meteor is Colliding with player", Play::cGreen, true);
		}

	}

	//VISIBILITY / POSITION CHECKS:
	//to see asteroid position
	for (int asteroidID : vAsteroids)
	{
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);
		Play::DrawDebugText({ asteroidObj.pos.x,asteroidObj.pos.y }, "Asteroid Here", Play::cGreen, true);
	}

	//to see meteor position
	for (int meteorID : vMeteor)
	{
		GameObject& meteorObj = Play::GetGameObject(meteorID);
		Play::DrawDebugText({ meteorObj.pos.x,meteorObj.pos.y }, "Meteor Here", Play::cGreen, true);
	}

	//to see gem position
	for (int gemID : vGem)
	{
		GameObject& gemObj = Play::GetGameObject(gemID);
		Play::DrawDebugText({ gemObj.pos.x,gemObj.pos.y }, "Gem Here", Play::cGreen, true);
	}

	//test to check if player is visible
	if (Play::IsVisible(playerObj))
	{
		Play::DrawDebugText({ playerObj.pos.x,playerObj.pos.y }, "Player is Here", Play::cGreen, true);
	}

	//ASTEROID CRICLE STUFF:
	//to see angle and distance between player and they are attached to asteroid
	GameObject& firstAsteroidObj = Play::GetGameObject(attachedAseteroidID);
	Play::DrawLine(playerObj.pos, firstAsteroidObj.pos, Play::cWhite);
}


//-----------------------------------------------------------------------------------------------------------------------------
//					                          PLAYER CONTROLS FUNCTIONS
//Player Controls when they are on an asteroid
void HandleGroundedControls() 
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	GameObject& attachedAsteroidObj = Play::GetGameObject(attachedAseteroidID);
	const float additionalRotation = 0.08;
	float playerToAsteroidAng = atan2f(playerObj.pos.y - attachedAsteroidObj.pos.y, playerObj.pos.x - attachedAsteroidObj.pos.x);
	playerObj.velocity = attachedAsteroidObj.velocity;
	playerObj.rotation = atan2f(playerObj.pos.y - attachedAsteroidObj.pos.y, playerObj.pos.x - attachedAsteroidObj.pos.x) + PLAY_PI / 2;

	if (gameState.playerState != stateDead)
	{
		if (Play::KeyDown(VK_RIGHT))
		{
			playerObj.pos.x = attachedAsteroidObj.pos.x + 65 * cos(playerToAsteroidAng + additionalRotation);
			playerObj.pos.y = attachedAsteroidObj.pos.y + 65 * sin(playerToAsteroidAng + additionalRotation);
			playerObj.rotation = atan2f(playerObj.pos.y - attachedAsteroidObj.pos.y, playerObj.pos.x - attachedAsteroidObj.pos.x) + PLAY_PI / 2;
			Play::SetSprite(playerObj, "agent8_right", 0.25f);
		}
		else if (Play::KeyDown(VK_LEFT))
		{
			playerObj.pos.x = attachedAsteroidObj.pos.x + 65 * cos(playerToAsteroidAng - additionalRotation);
			playerObj.pos.y = attachedAsteroidObj.pos.y + 65 * sin(playerToAsteroidAng - additionalRotation);
			playerObj.rotation = atan2f(playerObj.pos.y - attachedAsteroidObj.pos.y, playerObj.pos.x - attachedAsteroidObj.pos.x) + PLAY_PI / 2;
			Play::SetSprite(playerObj, "agent8_left", 0.25f);
		}
		else if (Play::KeyDown(VK_SPACE))
		{
			playerObj.rotation = playerToAsteroidAng + PLAY_PI / 2; // turn player so they are facing outwards
			playerObj.pos.x = attachedAsteroidObj.pos.x + 120 * cos(playerToAsteroidAng);
			playerObj.pos.y = attachedAsteroidObj.pos.y + 120 * sin(playerToAsteroidAng);
			Play::SetSprite(playerObj, "agent8_fly", 0.25f);
			Play::PlayAudio("explode");
			gameState.playerState = stateNotGrounded;
		}
		else 
		{
			Play::SetSprite(playerObj, "agent8_fly", 0.25f);
		}
	}
}

//Player Controls when they are not on an asteroid 
void HandleNotGroundedControls() 
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	const float additionalRotation = 0.08;

	if (gameState.playerState != stateDead)
	{
		playerObj.velocity = { 6,6 };

		if (Play::KeyDown(VK_RIGHT))
		{
			playerObj.rotation += additionalRotation;
		}

		if (Play::KeyDown(VK_LEFT))
		{
			playerObj.rotation -= additionalRotation;
		}
		playerObj.pos.x = playerObj.oldPos.x + sin(playerObj.rotation) * playerObj.velocity.x;
		playerObj.pos.y = playerObj.oldPos.y - cos(playerObj.rotation) * playerObj.velocity.y;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------
//					                          SPAWN FUNCTIONS
//Spawn a certain amount of asteroids
void SpawnAsteroids(int amount)
{
	for (int i = 0; i < amount; i++)
	{
		int asteroidID = Play::CreateGameObject(typeAsteroid, { Play::RandomRollRange(10,1270),Play::RandomRollRange(10,740) }, 45, "asteroid"); //Create Asteroid GameObject with a random y and x pos value
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);
		asteroidObj.velocity = Point2f(PickBetween(1, -1) * 2, Play::RandomRollRange(-1, 1) * 2);		// Randomise asteroidObj x and y velocity values so they are moving in different directions 

		if (asteroidObj.velocity == Point2f(0.0f, 0.0f)) // Sometimes asteroid spawns with 0 value for both x and y velocity so it wont move so need to give it a velocity
		{
			asteroidObj.velocity == Point2f(PickBetween(1, -1) * 2, PickBetween(1, -1) * 2);
		}

		//asteroidObj.scale = 0.9f;
		asteroidObj.rotation = std::atan2(asteroidObj.velocity.width, -asteroidObj.velocity.height); // find angle of rotation of asteroid with x and y velocity and assign it to that 
		Play::SetSprite(asteroidObj, "asteroid", 0.2f);
	}
}

//Spawn a certain amount of meteors
void SpawnMeteors(int amount)
{
	for (int i = 0; i < amount; i++)
	{
		int meteorID = Play::CreateGameObject(typeMeteor, { Play::RandomRollRange(10,1270),Play::RandomRollRange(10,740) }, 40, "meteor"); //Create meteor GameObject with a random y and x pos value
		GameObject& meteorObj = Play::GetGameObject(meteorID);
		meteorObj.velocity = Point2f(PickBetween(1, -1) * 2, Play::RandomRollRange(-1, 1) * 2);		// Randomise meteorObj x and y velocity values so they are moving in different directions 

		if (meteorObj.velocity.x == 0 && meteorObj.velocity.y == 0) // Sometimes meteor spawns with 0 value for both x and y velocity so it wont move so need to give it a velocity
		{
			meteorObj.velocity == Point2f(PickBetween(1, -1) * 2, PickBetween(1, -1) * 2);
		}

		meteorObj.rotation = std::atan2(meteorObj.velocity.width, -meteorObj.velocity.height); // find angle of rotation of asteroid with x and y velocity and assign it to that 
		Play::SetSprite(meteorObj, "meteor", 0.2f);
	}
}

//Spawn the player on the first asteroid on the list
//Used to spawn player on an asteroid at the start of a round
void SpawnPlayerOnAsteroid()
{
	GameObject& playerObj = Play::GetGameObjectByType(typePlayer);
	std::vector<int> vAsteroids = Play::CollectGameObjectIDsByType(typeAsteroid);
	GameObject& asteroidObj = Play::GetGameObject(vAsteroids.at(0)); //Get first in the attached list asteroid in list
	attachedAseteroidID = vAsteroids.at(0);

	//Put Player on a circular 'path' away from the first asteroid with correct rotation
	float playerToAsteroidAng = atan2f(playerObj.pos.y - asteroidObj.pos.y, playerObj.pos.x - asteroidObj.pos.x);
	playerObj.rotation = playerToAsteroidAng + PLAY_PI / 2; // turn player so they are facing outwards
	playerObj.pos.x = asteroidObj.pos.x + 65 * cos(playerToAsteroidAng);
	playerObj.pos.y = asteroidObj.pos.y + 65 * sin(playerToAsteroidAng);
	playerObj.velocity = asteroidObj.velocity;
	Play::SetSprite(playerObj, "agent8_fly", 0.25f);
}

//-----------------------------------------------------------------------------------------------------------------------------
//					                          UTILITY FUNCTIONS
// Screen Wraps object moving offscreen
void ScreenWrapping(GameObject& gameObject, bool choice)
{
	if (choice == true)
	{
		if (gameObject.oldPos.x < -50)
		{
			gameObject.pos.x = displayWidth + 50;
		}
		else if (gameObject.oldPos.x > displayWidth + 50)
		{
			gameObject.pos.x = -50;
		}

		if (gameObject.oldPos.y < -50)
		{
			gameObject.pos.y = displayHeight + 50;
		}
		else if (gameObject.oldPos.y > displayHeight + 50)
		{
			gameObject.pos.y = -50;
		}
	}
}

//Picks between 2 numbers because RandomRollRange also includes 0 so you can get 0,0 velocity
float PickBetween(float num1, float num2)
{
	float chance = Play::RandomRollRange(0, 50);

	if (chance < 25.0f)
	{
		return num1;
	}
	else {
		return num2;
	}
}

//Used to see if the player is on the asteroid
bool IsOnAsteroid() 
{
	GameObject& playerObj = Play::GetGameObject(typePlayer);
	std::vector<int> vAsteroid = Play::CollectGameObjectIDsByType(typeAsteroid);
	bool grounded = false;

	for (int asteroidID : vAsteroid)
	{
		GameObject& asteroidObj = Play::GetGameObject(asteroidID);

		if (Play::IsColliding(asteroidObj, playerObj))
		{
			grounded = true;
			return grounded;
		}
		else
		{
			return false;
		}
	}
}
