//Jay Stewart
// Assignment3_SceneSetup.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <sstream>
using namespace tle;

//constants 

const int kNumCheckpoints = 4;																					//number of each of the objects, used to spawn in the models and are used in collision detection 
const int kNumIsles = 16;
const int kNumWalls = 10;
const int kNumTanks1 = 8;
const int kNumTanks2 = 2;
const int kNumCollDummies = 1;


enum blockSide { side, frontBack, noSide };																		// type to identify which side of a barrier is being hit - 
																													//used in collision detection to have the car bounce off of the sides of the barrier

bool sphere2sphere(float dumXPos, float dumZPos, float dumRad, float isleXPos, float isleZPos, float isleRad);			

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

vector2D scalar(float s, vector2D v)													//look these up when I wake up
{
	return { s * v.x, s * v.z };
}

vector2D sum3(vector2D v1, vector2D v2, vector2D v3)
{
	return{ v1.x + v2.x + v3.x, v1.z + v2.z + v3.z };
}

//array initialisation
ModelCheckpoint checkpoint[kNumCheckpoints];											//arrays for all scenery
ModelIsle isle[kNumIsles];
ModelWall wall[kNumWalls];


void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine(kTLX);
	myEngine->StartWindowed();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder("./media");

	/**** Set up your scene here ****/
	float frameTime;																//multiplier for game speed
	float cameraMoveSpeed = 0.01f;													//used to move the camera 
	bool collision = false;															//used to determine whether there has been a collision or not
	float countdownTimer = 3.0f;													//timer that is used after the spacebar has been pressed to start the game	
	float boostTimer = 3.0f;														//timer for how long the boost can last, if space is held for longer than this the cooldown timer becomes active, a warning is also
																						//activated when it is within 1 second of the boost being overused
	float cooldownTimer = 5.0f;														//gives the boost key a cooldown 
	float stageCompleteTimer = 1.0f;												//used to display that a stage is complete when a checkpoint is passed 
	bool timerStart = false;														//used to determine if the space bar has been pressed to start the game 
	float warningTimer = 1.0f;														//displayed when the user needs to be warned that their booster is about to overheat 
	float goTimer = 1.0f;															//timer used to display go after the countdown timer has finished 
	int lap = 1;																	//used to output what lap the player is on when they pass the starting checkpoint 
	bool lapStart = false;															//makes sure that the race has already said go after the spacebar has been hit and the timer has finished 
																						//before it displays the current lap

	//game states
	enum gameStates { start, race, finished };																					//used to determine whether the player is in the starting stage, racing or has finished
	gameStates currentStateG = start;																								//contains movement, collision detection and checkpoint updates 

	//checkpoint numbers
	enum checkpointNumber { firstCheckpoint, secondCheckpoint, thirdCheckpoint, fourthCheckpoint, fifthCheckpoint, finish };	//used to determine what checkpoint the player is at
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
	IModel* cameraDummy = dummyMesh->CreateModel(0.0f, 0.0f, 0.0f);
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

	//attaching to parents
	myCamera->AttachToParent(cameraDummy);
	cameraDummy->AttachToParent(car);
	

	vector2D momentum{ 0.0f, 0.0f };
	vector2D thrust{ 0.0f, 0.0f };
	vector2D drag{ 0.0f, 0.0f };

	float matrix[4][4];

	// collision Dummies
	float collisionDummyZLocations[kNumCollDummies] = { 0.0f };

	// checkpoints
	float checkpointXLocations[kNumCheckpoints] = {0.0f, 0.0f, 100.0f, 100.0f };
	float checkpointZLocations[kNumCheckpoints] = {0.0f, 100.0f, 100.0f, 0.0f };

	// isles
	float isleXLocations[kNumIsles] = {-10.0f, 10.0f, 10.0f, -10.0f, 90.0f, 110.0f, 110.0f, 90.0f, 106.0f, 94.0f, 94.0f, 106.0f, 106.0f, 94.0f, 94.0f ,106.0f };
	float isleZLocations[kNumIsles] = {40.0f, 40.0f, 53.0f, 53.0f, 40.0f, 40.0f, 53.0f, 53.0f, -10.0f, -10.0f, -23.0f, -23.0f, -36.0f, -36.0f, -49.0f, -49.0f, };

	// tank 1 Locations
	float tank1XLocations[kNumTanks1] = {0.0f, 40.0f, 100.0f, 0.0f, 60.0f, 100, 0, 40};
	float tank1ZLocations[kNumTanks1] = {200.0f, 220.0f, 200.0f, -50.0f, -50.0f, -100.0f, -90.0f, -50.0f };

	// tank 2 Locations
	float tank2XLocations[kNumTanks2] = { 80.0f, 5.0f};
	float tank2ZLocations[kNumTanks2] = { 150.0f, 56.0f };

	// walls
	float wallXLocations[kNumWalls] = { -10.0f, 10.0f , 90.0f, 110.0f, 94 ,106, 94, 106, 94, 106 };
	float wallZLocations[kNumWalls] = { 46.0f, 46.0f , 46.0f, 46.0f, -16, -16, -42, -42, -30, -30 };

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
		tank2[i] = tank2Mesh->CreateModel(tank2XLocations[i], -8.0f, tank2ZLocations[i]);
		tank2[i]->RotateLocalZ(20);
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
					lapStart = true;
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

			outText << "lap " << ceilf(lap) << "/5";
			myFont->Draw(outText.str(), 800, 670);
			outText.str("");

			goTimer -= frameTime;

			if (goTimer > 0.0f)
			{
				myFont->Draw("Go!", 500, 670);
			}

			int MouseXMovement = myEngine->GetMouseMovementX();
			cameraDummy->RotateY(MouseXMovement);

			//save old position
			float carOldX = car->GetX();
			float carOldZ = car->GetZ();

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
				blockSide wallCollision = sphere2box(car->GetX(), car->GetZ(), carOldX, carOldZ, 2, wall[i]->GetX(), wall[i]->GetZ(), 4, 22);
				if (wallCollision == side || wallCollision == frontBack)
				{
					myFont->Draw("SIDE COLLISION", 200, 270);
					momentum.x = -momentum.x;
					momentum.z = -momentum.z;
					car->SetZ(carOldZ);
					car->SetX(carOldX);
				}
				else if (wallCollision == noSide)
				{
				}
			}

			for (int i = 0; i < kNumCheckpoints; i++)
			{
				for (int k = 0; k < kNumCollDummies; k++)
				{
					collision = sphere2sphere(car->GetX(), car->GetZ(), 2, checkpoint[i]->GetX() - 10, checkpoint[i]->GetZ(), 2);
					if (collision == true)
					{
						myFont->Draw("CHECKPOINT COLLISION", 300, 670);
						momentum.x = -momentum.x;
						momentum.z = -momentum.z;
						car->SetZ(carOldZ);
						car->SetX(carOldX);
					}
					collision = sphere2sphere(car->GetX(), car->GetZ(), 2, checkpoint[i]->GetX() + 10, checkpoint[i]->GetZ(), 2);
					if (collision == true)
					{
						myFont->Draw("CHECKPOINT COLLISION", 300, 670);
						momentum.x = -momentum.x;
						momentum.z = -momentum.z;
						car->SetZ(carOldZ);
						car->SetX(carOldX);
					}
				}
			}
			for (int i = 0; i < kNumTanks1; i++)
			{
				for (int k = 0; k < kNumCollDummies; k++)
				{
					collision = sphere2sphere(car->GetX(), car->GetZ(), 2, tank1[i]->GetX(), tank1[i]->GetZ(), 4);
					if (collision == true)
					{
						myFont->Draw("TANK COLLISION", 800, 670);
						momentum.x = -momentum.x;
						momentum.z = -momentum.z;
						car->SetZ(carOldZ);
						car->SetX(carOldX);
					}
				}
			}

			for (int i = 0; i < kNumTanks2; i++)
			{
				for (int k = 0; k < kNumCollDummies; k++)
				{
					collision = sphere2sphere(car->GetX(), car->GetZ(), 2, tank2[i]->GetX()-3, tank2[i]->GetZ(), 4);
					if (collision == true)
					{
						myFont->Draw("TANK COLLISION", 800, 670);
						momentum.x = -momentum.x;
						momentum.z = -momentum.z;
						car->SetZ(carOldZ);
						car->SetX(carOldX);
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
			
			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f && lapStart == true)
			{
				outText << "lap " << ceilf(lap) << "/5";
				myFont->Draw(outText.str(), 500, 170);
				outText.str("");
				

			}

			collision = sphere2point(car->GetX(), car->GetZ(), 2, checkpoint[1]->GetX(), checkpoint[1]->GetZ());
			if (collision == true)
			{
				stageCompleteTimer = 1;
				currentStateC = secondCheckpoint;
			}
			//reslove collisions
			break;
		}
		case secondCheckpoint:
		{
			myFont->Draw("Stage 2", 300, 670);

			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f)
			{
			
				myFont->Draw("Stage 1 Complete", 500, 170);
				
			}

			collision = sphere2point(car->GetX(), car->GetZ(), 2, checkpoint[2]->GetX(), checkpoint[2]->GetZ());
			if (collision == true)
			{
				stageCompleteTimer = 1;
				currentStateC = thirdCheckpoint;
			}
			break;
		}
		case thirdCheckpoint:
		{
			myFont->Draw("Stage 3", 300, 670);

			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f)
			{

				myFont->Draw("Stage 2 Complete", 500, 170);

			}

			collision = sphere2point(car->GetX(), car->GetZ(), 2, checkpoint[3]->GetX(), checkpoint[3]->GetZ());
			if (collision == true)
			{
				stageCompleteTimer = 1;
				currentStateC = fourthCheckpoint;
			}
			break;
		}
		case fourthCheckpoint:
		{
			myFont->Draw("Stage 4", 300, 670);
			
			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f)
			{

				myFont->Draw("Stage 3 Complete", 500, 170);

			}

			collision = sphere2point(car->GetX(), car->GetZ(), 2, checkpoint[0]->GetX(), checkpoint[0]->GetZ());
			if (collision == true)
			{
				if (lap == 5)
				{
					currentStateC = finish;
				}
				else
				{
					lap += 1;
					stageCompleteTimer = 1;
					currentStateC = firstCheckpoint;
				}
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



