//Jay Stewart
// Assignment3_SceneSetup.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
using namespace tle;

//constants 

const int kNumCheckpoints = 2;
const int kNumIsles = 4;
const int kNumWalls = 2;
const int kNumBuildings = 4;
const int kNumTanks1 = 1;
const int kNumTanks2 = 1;
const int kNumCollDummies = 3;


enum blockSide { leftSide, rightSide, frontSide, backSide, noSide };	// type to identify which side of a barrier is being hit - 
																		// used in collision detection to have the car bounce off of the sides of the barrier

bool sphere2sphere(float mXPos, float mZPos, float mRad, float bXPos, float bZPos, float bRad);			//sphere to sphere collision
blockSide sphere2box(float d1XPos, float d1ZPos, float d1OldXPos, float d1OldZPos, float d1Rad,
	float wXPos, float wZPos, float wWidth, float wDepth);

struct vector2D
{
	float x;
	float z;
};

struct ModelCheckpoint
{
	float checkpointX;
	float checkpointY;
	float checkpointZ;
	IModel* checkpoint;
};

struct ModelIsle
{
	float isleX;
	float isleY;
	float isleZ;
	IModel* isle;
};

struct ModelWall
{
	float wallX;
	float wallY;
	float wallZ;
	IModel* wall;
};

struct ModelBuilding
{
	float buildingX;
	float buildingY;
	float buildingZ;
	IModel* building;
};

vector2D scalar(float s, vector2D v)
{
	return { s * v.x, s * v.z };
}

vector2D sum3(vector2D v1, vector2D v2, vector2D v3)
{
	return{ v1.x + v2.x + v3.x, v1.z + v2.z + v3.z };
}

//array initialisation
ModelCheckpoint checkpoint[kNumCheckpoints];
ModelIsle isle[kNumIsles];
ModelWall wall[kNumWalls];
ModelBuilding building[kNumBuildings];


void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine(kTLX);
	myEngine->StartWindowed();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder("./media");

	/**** Set up your scene here ****/
	float pi = 3.1415f;
	float frameTime = myEngine->Timer();
	float radienDivision = pi / 180.0f;												//divides for the radien for vector movement
	float xVectorMovement;
	float zVectorMovement;
	float kAcceleration = 0.0f;
	float turningLowerLimit = -20.0f;
	float turningUpperLimit = 20.0f;
	float carRotation = 0.0f;
	float cameraMoveSpeed = 0.01f;
	float isleRow1X = -10.0f;
	float wallRow1X = -10.5f;
	bool collision = false;


	//game states
	enum gameStates { start, countdown, racing, finished };
	gameStates currentStateG = start;

	//checkpoint numbers
	enum checkpointNumber { firstCheckpoint, secondCheckpoint /*from here keep adding checkpoints (4 checkpoints including finish line for 70%)*/, finish };
	checkpointNumber currentStateC = firstCheckpoint;

	//controls
	const EKeyCode kAccelerateKey = Key_W;
	const EKeyCode kDecelerateKey = Key_S;
	const EKeyCode kAntiClockwiseTurnKey = Key_A;
	const EKeyCode kClockwiseTurnKey = Key_D;
	const EKeyCode kBoostKey = Key_Space;
	const EKeyCode kCameraForwardKey = Key_Up;
	const EKeyCode kCameraBackKey = Key_Down;
	const EKeyCode kCameraLeftKey = Key_Left;
	const EKeyCode kCameraRightKey = Key_Right;
	const EKeyCode kCameraResetKey = Key_1;
	const EKeyCode kFirstPersonCameraKey = Key_2;


	//checkpoint variables
	int checkpointNumber = 0;


	//player car variables
	float reverseSpeedMultiplier = 4.0f;



	//meshes
	IMesh* skyboxMesh = myEngine->LoadMesh("Skybox 07.x");
	IMesh* carMesh = myEngine->LoadMesh("race2.x");
	IMesh* floorMesh = myEngine->LoadMesh("ground.x");
	IMesh* isleMesh = myEngine->LoadMesh("IsleStraight.x");
	IMesh* wallMesh = myEngine->LoadMesh("Wall.x");
	IMesh* tank1Mesh = myEngine->LoadMesh("TankSmall1.x");
	IMesh* tank2Mesh = myEngine->LoadMesh("TankSmall2.x");
	IMesh* enemyMesh = myEngine->LoadMesh("Interstellar.x");
	IMesh* checkpointMesh = myEngine->LoadMesh("Checkpoint.x");
	IMesh* enemyCarMesh = myEngine->LoadMesh("Interstellar.x");
	IMesh* dummyMesh = myEngine->LoadMesh("Dummy.x");






	//models
	IModel* isle[kNumIsles];
	IModel* wall[kNumWalls];
	IModel* tank1[kNumTanks1];
	IModel* tank2[kNumTanks2];
	IModel* checkpoint[kNumCheckpoints];
	IModel* collisionDummy[kNumCollDummies];
	IModel* skybox = skyboxMesh->CreateModel(0.0f, -960.0f, 0.0f);
	IModel* floor = floorMesh->CreateModel(0.0f, 0.0f, 0.0f);
	IModel* car = carMesh->CreateModel(0.0f, 0.0f, 0.0f);

	//fonts
	IFont* myFont = myEngine->LoadFont("Comic Sans MS", 36);

	//ui
	ISprite* backdrop;
	backdrop = myEngine->CreateSprite("ui_backdrop.jpg");
	backdrop->SetPosition(300, 660);

	//camera
	ICamera* myCamera = myEngine->CreateCamera(kManual, 0.00f, 5.0f, -20.0f);
	myCamera->AttachToParent(car);
	

	vector2D momentum{ 0.0f, 0.0f };
	vector2D thrust{ 0.0f, 0.0f };
	vector2D drag{ 0.0f, 0.0f };

	float matrix[4][4];

	// model posititons
	// collision Dummies
	float collisionDummyZLocations[kNumCollDummies] = { 4.0f, 0.0f , -4.0f };

	// checkpoints
	float checkpointXLocations[kNumCheckpoints] = {};
	float checkpointZLocations[kNumCheckpoints] = {};

	// isles
	float isleXLocations[kNumIsles] = {};
	float isleZLocations[kNumIsles] = {};

	// tank 1 Locations
	float tank1XLocations[kNumTanks1] = {};
	float tank1ZLocations[kNumTanks1] = {};

	// tank 2 Locations
	float tank2XLocations[kNumTanks2] = {};
	float tank2ZLocations[kNumTanks2] = {};

	// walls
	float wallXLocations[kNumWalls] = {};
	float wallZLocations[kNumWalls] = {};


	for (int i = 0; i < kNumCollDummies; i++)
	{
		collisionDummy[i] = dummyMesh->CreateModel( 0.0f, 0.0f, collisionDummyZLocations[i]);
	}

	for (int i = 0; i < kNumWalls; i++)
	{
		wall[i] = wallMesh->CreateModel( wallXLocations[i], 0.0f, wallZLocations[i]);
	}

	for (int i = 0; i < kNumIsles; i++)
	{
		isle[i] = isleMesh->CreateModel(isleXLocations[i], 0.0f, isleZLocations[i]);
	}

	for (int i = 0; i < kNumCheckpoints; i++)
	{
		checkpoint[i] = checkpointMesh->CreateModel( checkpointXLocations[i], 0.0f, checkpointZLocations[i]);
	}

	for (int i = 0; i < kNumTanks1; i++)
	{
		tank1[i] = tank1Mesh->CreateModel( tank1XLocations[i], 0.0f, tank1ZLocations[i]);
	}

	for (int i = 0; i < kNumTanks2; i++)
	{
		tank2[i] = tank2Mesh->CreateModel(tank2XLocations[i], 0.0f, tank2ZLocations[i]);
	}

	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning())
	{
		// Draw the scene
		myEngine->DrawScene();

		/**** Update your scene each frame here ****/

		//start of switch currentStateG
		switch (currentStateG)
		{
		case start:
		{
			// in the UI have the bottom screen say "hit space to start"
			myFont->Draw("Press space to start", 500, 670);
			if (myEngine->KeyHit(kBoostKey))
			{
				currentStateG = countdown;
			}
			break;
		}

		case countdown:
		{
			//flash "3" "2" "1" "Go!" when space is hit
			for (int i = 3; i > 0; i--)
			{
				myFont->Draw(" ", 500, 670);
			}
		}
		}



		//start of switch currentStateC
		switch (currentStateC)
		{
		case firstCheckpoint:
		{
			// switch to firstCheckpoint state after 
			//display press space to start here 
			if (myEngine->KeyHit(kBoostKey))
			{
				//flash 3, 2, 1, go goes here, should check whether the output is go and transition to first checkpoint when that happens 
				currentStateC = secondCheckpoint;
			}
			break;
		}

		case secondCheckpoint:
		{
			// in the UI at the bottom display "stage 1 complete"
			myFont->Draw("you can move now", 500, 670);
			//save old position
			float dummy1OldX = collisionDummy[0]->GetX();
			float dummy1OldZ = collisionDummy[0]->GetZ();

			//move the models
			// get the facing vector
			car->GetMatrix(&matrix[0][0]);
			vector2D facingVector = { matrix[2][0], matrix[2][2] };
			// calculate thrust based on keyboard input 

			if (myEngine->KeyHeld(kAntiClockwiseTurnKey)) car->RotateY(-0.05f);
			if (myEngine->KeyHeld(kClockwiseTurnKey)) car->RotateY(0.05f);

			if (myEngine->KeyHeld(kAccelerateKey))
			{
				thrust = scalar(0.00001f, facingVector);							//magic number is thrust factor 
			}
			else if (myEngine->KeyHeld(kDecelerateKey))
			{
				thrust = scalar(-0.00001f, facingVector);
			}
			else
			{
				thrust = { 0.0f, 0.0f };
			}
			// calculate drag based off of previous momentum

			drag = scalar(-0.0001, momentum);

			// calculate momentum based on thrust, drag and previous momentum
			momentum = sum3(momentum, thrust, drag);
			// move the car depending on the new momentum
			car->Move(momentum.x, 0.0f, momentum.z);

			if (myEngine->KeyHeld(kCameraForwardKey))
			{
				myCamera->MoveLocalZ(cameraMoveSpeed);
			}

			if (myEngine->KeyHeld(kCameraBackKey))
			{
				myCamera->MoveLocalZ(-cameraMoveSpeed);
			}

			if (myEngine->KeyHeld(kCameraLeftKey))
			{
				myCamera->MoveLocalX(-cameraMoveSpeed);
			}

			if (myEngine->KeyHeld(kCameraRightKey))
			{
				myCamera->MoveLocalX(cameraMoveSpeed);
			}

			if (myEngine->KeyHit(kCameraResetKey))
			{
				myCamera->SetLocalPosition(0, 5, -20);

			}

			if (myEngine->KeyHit(kFirstPersonCameraKey))
			{
				myCamera->SetLocalPosition(0, 5, 0);
			}

			//check for collisions
			//float dXPos, float dZPos, float dRad, float cXPos, float cZPos, float cRad

			for (int i = 0; i < kNumWalls; i++)
			{
				for (int k = 0; k < kNumCollDummies; k++)
				{
					/*collision = sphere2sphere(collisionDummy[k]->GetX(), collisionDummy[k]->GetZ(), 2, wall[i].wall->GetX(), wall[i].wall->GetZ(), 2);
					if (collision == true)
					{
						myFont->Draw("COLLISION", 300, 670);
					}*/
				}
			}
			//reslove collisions
			break;
		}
		//add more game states here 
		case finish:
		{
			// in the UI at the bottom display "race complete"
			break;
		}
		}
		//end of switch currentStateG


		if (myEngine->KeyHeld(Key_Escape))								// if the user presses escape it will shut down the game
		{
			myEngine->Stop();
		}

	}

	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}

bool sphere2sphere(float dXPos, float dZPos, float dRad, float cXPos, float cZPos, float cRad)		//function created for the eventual implementation of sphere to sphere collision between marbles - 
{																										//it is currently set up to collide with boxes but is not called anywhere in the code
	float distX = cXPos - dXPos;
	float distZ = cZPos - dZPos;
	float distance = sqrt(distX*distX + distZ * distZ);

	return(distance < (dRad + cRad));
}

blockSide sphere2box(float d1XPos, float d1ZPos, float d1OldXPos, float d1OldZPos, float d1Rad, float wXPos, float wZPos, float wWidth, float wDepth)	//retrives values from where it is called
{
	float minX = wXPos - wWidth - d1Rad;			//creation of a box around blocks to determine if the car dummies have collided by adding the radius of the marble to the radius of the block
	float maxX = wXPos + wWidth + d1Rad;
	float minZ = wZPos - wDepth - d1Rad;
	float maxZ = wZPos + wDepth + d1Rad;

	blockSide result = noSide;

	if (d1XPos > minX && d1XPos < maxX && d1ZPos > minZ && d1ZPos < maxZ)	//outputs which side the marble has hit to where the function has been called to change vectors appropriately
	{
		if (d1OldXPos < minX)
		{
			result = leftSide;
		}
		else if (d1OldXPos > maxX)
		{
			result = rightSide;
		}
		else if (d1OldZPos < minZ)
		{
			result = frontSide;
		}
		else
		{
			result = backSide;
		}
	}


	return(result);
}



