//Jay Stewart
// Assignment3_SceneSetup.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <sstream>
using namespace tle;

//constants 

const int kNumCheckpoints = 4;
const int kNumIsles = 8;
const int kNumWalls = 4;
const int kNumBuildings = 4;
const int kNumTanks1 = 6;
const int kNumTanks2 = 1;
const int kNumCollDummies = 3;


enum blockSide { side, frontBack, noSide };	// type to identify which side of a barrier is being hit - 
																		// used in collision detection to have the car bounce off of the sides of the barrier

bool sphere2sphere(float dumXPos, float dumZPos, float dumRad, float isleXPos, float isleZPos, float isleRad);			//sphere to sphere collision

blockSide sphere2box(float d1XPos, float d1ZPos, float d1OldXPos, float d1OldZPos, float d1Rad,
	float wXPos, float wZPos, float wWidth, float wDepth);

bool sphere2point(float dumXPos, float dumZPos, float dumRad, float checkXPos, float checkZPos);

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
	float frameTime;
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
	float countdownTimer = 3.0f;
	float boostTimer = 3.0f;
	float cooldownTimer = 5.0f;
	float stageCompleteTimer = 1.0f;
	float lapTimer = 0.0f;
	bool timerStart = false;
	float warningTimer = 1.0f;
	float goTimer = 1.0f;


	//game states
	enum gameStates { start, race, finished };
	gameStates currentStateG = start;

	//checkpoint numbers
	enum checkpointNumber { firstCheckpoint, secondCheckpoint, thirdCheckpoint, fourthCheckpoint, fifthCheckpoint, finish };
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

	//text
	stringstream outText;

	//ui
	ISprite* backdrop;
	backdrop = myEngine->CreateSprite("ui_backdrop.jpg");
	backdrop->SetPosition(300, 660);

	//camera
	ICamera* myCamera = myEngine->CreateCamera(kManual, 0.0f, 7.0f, -20.0f);
	myCamera->AttachToParent(car);
	

	vector2D momentum{ 0.0f, 0.0f };
	vector2D thrust{ 0.0f, 0.0f };
	vector2D drag{ 0.0f, 0.0f };

	float matrix[4][4];

	// model posititons

	// collision Dummies
	float collisionDummyZLocations[kNumCollDummies] = { 4.0f, 0.0f , -4.0f };

	// checkpoints
	float checkpointXLocations[kNumCheckpoints] = {0.0f, 0.0f, 100.0f, 100.0f };
	float checkpointZLocations[kNumCheckpoints] = {0.0f, 100.0f, 100.0f, 0.0f };

	// isles
	float isleXLocations[kNumIsles] = {-10.0f, 10.0f, 10.0f, -10.0f, 90.0f, 110.0f, 110.0f, 90.0f};
	float isleZLocations[kNumIsles] = {40.0f, 40.0f, 53.0f, 53.0f, 40.0f, 40.0f, 53.0f, 53.0f };

	// tank 1 Locations
	float tank1XLocations[kNumTanks1] = {0.0f, 40.0f, 100.0f, 0.0f, 40.0f, 100.0f };
	float tank1ZLocations[kNumTanks1] = {200.0f, 220.0f, 200.0f, -50.0f, -75.0f, -50.0f};

	// tank 2 Locations
	float tank2XLocations[kNumTanks2] = { 100.0f };
	float tank2ZLocations[kNumTanks2] = { 200.0f };

	// walls
	float wallXLocations[kNumWalls] = { -10.0f, 10.0f , 90.0f, 110.0f };
	float wallZLocations[kNumWalls] = { 46.0f, 46.0f , 46.0f, 46.0f };


	for (int i = 0; i < kNumCollDummies; i++)
	{
		collisionDummy[i] = dummyMesh->CreateModel( 0.0f, 0.0f, collisionDummyZLocations[i]);
		collisionDummy[i]->AttachToParent(car);
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
		frameTime = myEngine->Timer();
		

		//start of switch currentStateG
		switch (currentStateG)
		{
		case start:
		{
			// in the UI have the bottom screen say "hit space to start"
			if (timerStart == false)
			{
				myFont->Draw("Press space to start", 500, 670);
				if (myEngine->KeyHit(kBoostKey))
				{

					timerStart = true;

				}
			}

			//flash "3" "2" "1" "Go!" when space is hit
			if (timerStart == true)
			{
				countdownTimer -= frameTime;
				if (countdownTimer > 0.0f)
				{
					outText << ceilf(countdownTimer);
					myFont->Draw(outText.str(), 500, 670);
					outText.str("");
				}
				else
				{
					//move to race state and make lifecycle counter (1s)
					myFont->Draw("Go!", 500, 670);

					currentStateG = race;

				}
			}
			break;
		}

		case race:
		{
			float speedometer = sqrt((momentum.x*momentum.x) + (momentum.z*momentum.z));
			outText << ceilf(speedometer) << "mph";
			myFont->Draw(outText.str(), 900, 670);
			outText.str("");

			goTimer -= frameTime;

			if (goTimer > 0.0f)
			{
				myFont->Draw("Go!", 500, 670);
			}

			//save old position
			float dummy1OldX = collisionDummy[0]->GetX();
			float dummy1OldZ = collisionDummy[0]->GetZ();
			float dummy2OldX = collisionDummy[1]->GetX();
			float dummy2OldZ = collisionDummy[1]->GetZ();
			float dummy3OldX = collisionDummy[2]->GetX();
			float dummy3OldZ = collisionDummy[2]->GetZ();

			//move the models
			// get the facing vector
			car->GetMatrix(&matrix[0][0]);
			vector2D facingVector = { matrix[2][0], matrix[2][2] };
			// calculate thrust based on keyboard input 

			if (myEngine->KeyHeld(kAntiClockwiseTurnKey)) car->RotateY(-0.05f);
			if (myEngine->KeyHeld(kClockwiseTurnKey)) car->RotateY(0.05f);

			if (myEngine->KeyHeld(kAccelerateKey))
			{
				thrust = scalar(80.0f * frameTime, facingVector);							//magic number is thrust factor 
			}
			else if (myEngine->KeyHeld(kDecelerateKey))
			{
				thrust = scalar(-40.0f * frameTime, facingVector);
			}
			else
			{
				thrust = { 0.0f, 0.0f };
			}
			// calculate drag based off of previous momentum

			drag = scalar(-1.0*frameTime, momentum);

			// calculate momentum based on thrust, drag and previous momentum
			momentum = sum3(momentum, thrust, drag);
			// move the car depending on the new momentum
			car->Move(momentum.x*frameTime, 0.0f, momentum.z*frameTime);

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
				myCamera->SetLocalPosition(0, 7, -20);

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
					blockSide wallCollision = sphere2box(collisionDummy[k]->GetX(), collisionDummy[k]->GetZ(), dummy1OldX, dummy1OldZ, 2, wall[i]->GetX(), wall[i]->GetZ(), 4, 22);
					if (wallCollision == side)
					{
						myFont->Draw("SIDE COLLISION", 200, 270);
						momentum.x = -momentum.x;
						momentum.z = -momentum.z;
					}
					if (wallCollision == frontBack)
					{
						myFont->Draw("FRONTBACK COLLISION", 210, 370);
						momentum.x = -momentum.x;
						momentum.z = -momentum.z;
					}
					if (wallCollision == noSide)
					{
					}
				}
			}

			for (int i = 0; i < kNumCheckpoints; i++)
			{
				for (int k = 0; k < kNumCollDummies; k++)
				{
					collision = sphere2sphere(collisionDummy[k]->GetX(), collisionDummy[k]->GetZ(), 2, checkpoint[i]->GetX() - 10, checkpoint[i]->GetZ(), 2);
					if (collision == true)
					{
						myFont->Draw("CHECKPOINT COLLISION", 300, 670);
					}
					collision = sphere2sphere(collisionDummy[k]->GetX(), collisionDummy[k]->GetZ(), 2, checkpoint[i]->GetX() + 10, checkpoint[i]->GetZ(), 2);
					if (collision == true)
					{
						myFont->Draw("CHECKPOINT COLLISION", 300, 670);
					}
				}
			}
			for (int i = 0; i < kNumTanks1; i++)
			{
				for (int k = 0; k < kNumCollDummies; k++)
				{
					collision = sphere2sphere(collisionDummy[k]->GetX(), collisionDummy[k]->GetZ(), 2, tank1[i]->GetX(), tank1[i]->GetZ(), 4);
					if (collision == true)
					{
						myFont->Draw("TANK COLLISION", 800, 670);
					}
				}
			}
			break;
		}
		case finished:
		{
		}
		}



		//start of switch currentStateC
		switch (currentStateC)
		{
		case firstCheckpoint:
		{
			// in the UI at the bottom display "stage 1 complete"
			myFont->Draw("Stage 1", 300, 670);
			
			collision = sphere2point(collisionDummy[1]->GetX(), collisionDummy[1]->GetZ(), 2, checkpoint[1]->GetX(), checkpoint[1]->GetZ());
			if (collision == true)
			{
				currentStateC = secondCheckpoint;
			}
			//reslove collisions
			break;
		}
		case secondCheckpoint:
		{
			myFont->Draw("Stage 2", 300, 670);

			collision = sphere2point(collisionDummy[1]->GetX(), collisionDummy[1]->GetZ(), 2, checkpoint[2]->GetX(), checkpoint[2]->GetZ());
			if (collision == true)
			{
				currentStateC = thirdCheckpoint;
			}
			break;
		}
		case thirdCheckpoint:
		{
			myFont->Draw("Stage 3", 300, 670);

			collision = sphere2point(collisionDummy[1]->GetX(), collisionDummy[1]->GetZ(), 2, checkpoint[3]->GetX(), checkpoint[3]->GetZ());
			if (collision == true)
			{
				currentStateC = fourthCheckpoint;
			}
			break;
		}
		case fourthCheckpoint:
		{
			myFont->Draw("Stage 4", 300, 670);
			

			collision = sphere2point(collisionDummy[1]->GetX(), collisionDummy[1]->GetZ(), 2, checkpoint[0]->GetX(), checkpoint[0]->GetZ());
			if (collision == true)
			{
				currentStateC = finish;
			}

			break;
		}
		//add more game states here 
		case finish:
		{
			// in the UI at the bottom display "race complete"

			myFont->Draw("Race Complete", 500, 670);
			currentStateG = finished;
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

bool sphere2sphere(float dumXPos, float dumZPos, float dumRad, float isleXPos, float isleZPos, float isleRad)		//function created for the eventual implementation of sphere to sphere collision between marbles - 
{																										//it is currently set up to collide with boxes but is not called anywhere in the code
	float distX = isleXPos - dumXPos;
	float distZ = isleZPos - dumZPos;
	float distance = sqrt(distX*distX + distZ * distZ);

	return(distance < (dumRad + isleRad));
}

bool sphere2point(float dumXPos, float dumZPos, float dumRad, float checkXPos, float checkZPos)
{
	float checkMinX = checkXPos - 5;
	float checkMaxX = checkXPos + 5;
	float checkMinZ = checkZPos - 2;
	float checkMaxZ = checkZPos + 2;

	if (dumXPos > checkMinX && dumXPos < checkMaxX && dumZPos >= checkMinZ && dumZPos <= checkMaxZ)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}


blockSide sphere2box(float d1XPos, float d1ZPos, float d1OldXPos, float d1OldZPos, float d1Rad, float wXPos, float wZPos, float wWidth, float wDepth)	//retrives values from where it is called
{
	float minX = wXPos - wWidth / 2 - d1Rad;			//creation of a box around blocks to determine if the car dummies have collided by adding the radius of the marble to the radius of the block
	float maxX = wXPos + wWidth / 2 + d1Rad;
	float minZ = wZPos - wDepth / 2 - d1Rad;
	float maxZ = wZPos + wDepth / 2 + d1Rad;

	blockSide result = noSide;

	if (d1XPos > minX && d1XPos < maxX && d1ZPos > minZ && d1ZPos < maxZ)	//outputs which side the marble has hit to where the function has been called to change vectors appropriately
	{
		if (d1OldXPos < minX || d1OldXPos > maxX)
		{
			result = side;
		}
		else
		{
			result = frontBack;
		}
	}


	return(result);
}



