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

vector2D scalar(float s, vector2D v)													
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

	//Variables
	//game 
	int numberOfLaps = 5;															//the amount of laps the car has to do before the game will end
	bool carDestroyed = false;														//if the car runs out of health this will trigger the game to end 
	float dragFactor = -1.0f;														//how much the car is slowed down by default just by moving, this is used with thrust to calculate momentum
	float forwardsThrustFactor = 80.0f;												//forward thrust of the car, used to calculate momentum and used in the boost to speed up the car or slow it down
	float boostMultiplier = 2.0f;													//how much the car is sped up by when boosting, is multiplied with the trust factors
	float reverseThrustFactor = -forwardsThrustFactor / 2;							//how much thrust there is when reversing, half of the forward thrust factor 
	float overheatedMultiplier = 2.0f;												//how much the car slows down by if the boost overheats it 
	int damagePerHit = 1;															//how much damage done per collision with something 
	bool boostActive = false;														//determine whether the boost is active or not and therefore what speed the car should be moving at 
	bool onCooldown = false;														//whether the boos should be on cooldown or not, changed when boost timer hits 0 and triggers a delay
	bool lapStart = false;															//makes sure that the race has already said go after the spacebar has been hit and the timer has finished 
																						//before it displays the current lap
	int lap = 1;																	//used to output what lap the player is on when they pass the starting checkpoint 
	bool timerStart = false;														//used to determine if the space bar has been pressed to start the game 
	float frameTime;																//multiplier for game speed

	//timers
	float boostRechargeTimerLimit = 2.0f;											//to what second should the timer recharge until if some of the boost has been used 
	float stageCompletionTimerLength = 1.0f;										//how long the stage completion popup should stay
	float overheatWarningTime = 1.0f;												//how long before overheating should the warning popup come up 
	float warningTimer = 1.0f;														//displayed when the user needs to be warned that their booster is about to overheat 
	float goTimer = 1.0f;															//timer used to display go after the countdown timer has finished 
	float countdownTimer = 3.0f;													//timer that is used after the spacebar has been pressed to start the game	
	float boostTimer = 3.0f;														//timer for how long the boost can last, if space is held for longer than this the cooldown timer becomes active, a warning is also
																						//activated when it is within 1 second of the boost being overused
	float cooldownTimer = 5.0f;														//gives the boost key a cooldown 
	float cooldownLength = 5.0f;													//how long the cooldown should be until the player is allowed to boost again 
	float stageCompleteTimer = 1.0f;												//used to display that a stage is complete when a checkpoint is passed 

	//camera
	int camera3rdPersonXPosition = 0;												//positions of the camera in third person
	int camera3rdPersonYPosition = 7;
	int camera3rdPersonZPosition = -20;
	int camera1stPersonXPosition = 0;												//positions of the camera in first person 
	int camera1stPersonYPosition = 5;
	int camera1stPersonZPosition = 0;
	float cameraMoveSpeed = 0.01f;													//used to move the camera 

	//object Variables 
	//car
	int carDamage = 100;															//how much health the car has until it totals, this has the damage taken away from it every collision 
	float checkpointLegDistence = 9.0f;												//the distence between the middle of the checkpoint and the legs, used in collision with the legs 
	float checkpointLegRadius = 2.0f;												//used to make sphere to sphere collisions with the car and checkpoint legs
	float tank2YPos = -8.0f;														//the position of the tanks that go underground 
	float tankRadius = 4.0f;														//used in sphere to sphere collision with the tanks 
	float tankRotationHitboxOffset = 3.0f;											//used to correct hitboxes in tanks that are submerged and tilted 
	float carRadius = 2.0f;															//used to determine the cars hitbox with all collisions 
	float wallWidth = 4.0f;															//used to determine hitboxes of the walls and isles 
	float wallDepth = 22.0f;														//^
	float carRotationLimit = 0.05f;													//the limits to how fast the car can rotate 
	bool collision = false;															//used to determine whether there has been a collision or not


	//text variables																//the contents and location of all text in the game
	//start text
	string startText = "Press space to start";
	int startTextXLocation = 500;
	int startTextYLocation = 670;

	//countdown Timer 
	int countdownTimerXLocation = 500;
	int countdownTimerYLocation = 670;

	//speedometer
	string mphText = "mph";
	int mphTextXLocation = 900;
	int mphTextYLocation = 670;

	//health
	string healthText = " health";
	int healthTextXLocation = 650;
	int healthTextYLocation = 670;

	//laps
	string lapText = "lap ";
	string numberOfLapsText = "/5";
	int lapTextXLocation = 800;
	int lapTextYLocation = 670;

	//go!
	string goText = "Go!";
	int goTextXLocation = 500;
	int goTextYLocation = 670;

	//overheating
	string overheatingText = "BOOST OVERHEATING!";
	int overheatingTextXLocation = 500;
	int overheatingTextYLocation = 250;

	//overheated
	string overheatedText = "OVERHEATED!";
	int overheatedTextXLocation = 500;
	int overheatedTextYLocation = 250;

	//totaled 
	string totaledText = "Your car has been totaled";
	int totaledTextXLocation = 500;
	int totaledTextYLocation = 670;

	//stages
	string stage1Text = "Stage 1";
	string stage2Text = "Stage 2";
	string stage3Text = "Stage 3";
	string stage4Text = "Stage 4";
	int stagesTextXLocation = 300;
	int stagesTextYLocation = 670;

	//stage Complete
	string stage1CompleteText = "Stage 1 Complete";
	string stage2CompleteText = "Stage 2 Complete";
	string stage3CompleteText = "Stage 3 Complete";
	int stageCompleteTextXLocation = 500;
	int stageCompleteTextYLocation = 170;

	//lap Complete
	int lapNumberPopupXLocation = 500;
	int lapNumberPopupYLocation = 200;

	//race Complete
	string raceCompleteText = "Race Complete";
	int raceCompleteTextXLocation = 500;
	int raceCompleteTextYLocation = 670;



	//game states
	enum gameStates { start, race, finished };																					//used to determine whether the player is in the starting stage, racing or has finished
	gameStates currentStateG = start;																								//contains movement, collision detection and checkpoint updates 

	//checkpoint numbers
	enum checkpointNumber { firstCheckpoint, secondCheckpoint, thirdCheckpoint, fourthCheckpoint, fifthCheckpoint, finish };	//used to determine what checkpoint the player is at
	checkpointNumber currentStateC = firstCheckpoint;

	//controls
	const EKeyCode kAccelerateKey = Key_W;												//accelerates the car forward
	const EKeyCode kDecelerateKey = Key_S;												//decelerates the car backwards
	const EKeyCode kAntiClockwiseTurnKey = Key_A;										//turns the car anticlockwise
	const EKeyCode kClockwiseTurnKey = Key_D;											//turns the car clockwise
	const EKeyCode kBoostKey = Key_Space;												//speeds up the car for a certain amount of time then overheats, slowing down the car if used too long 
	const EKeyCode kCameraForwardKey = Key_Up;											//moves the camera forward
	const EKeyCode kCameraBackKey = Key_Down;											//moves the camera backwards
	const EKeyCode kCameraLeftKey = Key_Left;											//moves the camera left
	const EKeyCode kCameraRightKey = Key_Right;											//moves the camera left
	const EKeyCode kCameraResetKey = Key_1;												//resets the camera to the 3rd person perspective
	const EKeyCode kFirstPersonCameraKey = Key_2;										//sets the camera to the 1st person perspective 

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
	IFont* myFont = myEngine->LoadFont("Comic Sans MS", 36);								//the font and size of all text used 

	//text
	stringstream outText;

	//ui
	ISprite* backdrop;
	backdrop = myEngine->CreateSprite("ui_backdrop.jpg");
	backdrop->SetPosition(300, 660);

	//camera
	ICamera* myCamera = myEngine->CreateCamera(kManual, camera3rdPersonXPosition, camera3rdPersonYPosition, camera3rdPersonZPosition);

	//attaching to parents
	myCamera->AttachToParent(cameraDummy);
	cameraDummy->AttachToParent(car);
	

	vector2D momentum{ 0.0f, 0.0f };
	vector2D thrust{ 0.0f, 0.0f };
	vector2D drag{ 0.0f, 0.0f };

	float matrix[4][4];

	// collision Dummies
	float collisionDummyZLocations[kNumCollDummies] = { 0.0f };

	// checkpoints									{1}		{2}		{3}		{4}
	float checkpointXLocations[kNumCheckpoints] = {	0.0f,	0.0f,	100.0f, 100.0f };
	float checkpointZLocations[kNumCheckpoints] = {	0.0f,	100.0f, 100.0f, 0.0f };

	// isles							{1}		{2}		{3}		{4}		{5}		{6}		{7}		{8}		{9}		{10}	{11}	{12}	{13}	{14}	{15}	{16}
	float isleXLocations[kNumIsles] = {	-10.0f,	10.0f,	10.0f,	-10.0f,	90.0f,	110.0f, 110.0f,	90.0f,	106.0f,	94.0f,	94.0f,	106.0f,	106.0f,	94.0f,	94.0f,	106.0f };
	float isleZLocations[kNumIsles] = {	40.0f,	40.0f,	53.0f,	53.0f,	40.0f,	40.0f,	53.0f,	53.0f,	-10.0f, -10.0f,	-23.0f, -23.0f, -36.0f,	-36.0f, -49.0f, -49.0f, };

	// tank 1 Locations						{1}		{2}		{3}		{4}		{5}		{6}		 {7}	 {8}
	float tank1XLocations[kNumTanks1] = {	0.0f,	40.0f,	100.0f,	0.0f,	60.0f,	100.0f,	 0.0f,	 40.0f };
	float tank1ZLocations[kNumTanks1] = {	200.0f, 220.0f, 200.0f, -50.0f, -50.0f, -100.0f, -90.0f, -50.0f };

	// tank 2 Locations					  {1}		{2}		
	float tank2XLocations[kNumTanks2] = { 80.0f,	5.0f};
	float tank2ZLocations[kNumTanks2] = { 150.0f,	80.0f };

	// walls							{1}		{2}		{3}		{4}		{5}		{6}		{7}		{8}		{9}		{10}
	float wallXLocations[kNumWalls] = { -10.0f, 10.0f , 90.0f,	110.0f,	94.0f,  106.0f, 94.0f,  106.0f,	94.0f,	106.0f };
	float wallZLocations[kNumWalls] = {	46.0f,	46.0f ,	46.0f,	46.0f,	-16.0f, -16.0f, -42.0f, -42.0f,	-30.0f,	-30.0f };

	//creation of all walls
	for (int i = 0; i < kNumWalls; i++)																				//uses arrays with locations in to create each of the models 
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
		tank2[i] = tank2Mesh->CreateModel(tank2XLocations[i], tank2YPos, tank2ZLocations[i]);
		tank2[i]->RotateLocalZ(20);
	}

	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning())
	{
		// Draw the scene
		myEngine->DrawScene();

		/**** Update your scene each frame here ****/
		frameTime = myEngine->Timer();															//updates frametime each of the loops for an accurate game speed 
		

		//start of switch currentStateG
		switch (currentStateG)
		{
		case start:
		{
			// in the UI have the bottom screen say "hit space to start"
			if (timerStart == false)															//if the game has not yet been started this has you press space to start and starts the countdown timer 													
			{
				myFont->Draw( startText , startTextXLocation, startTextYLocation);
				if (myEngine->KeyHit(kBoostKey))
				{

					timerStart = true;

				}
			}

			//flash "3" "2" "1" "Go!" when space is hit
			if (timerStart == true)															//counts down when space has been hit, displays 3, 2, 1 then transitions the game to the race state and sets the lap to start
			{
				countdownTimer -= frameTime;
				if (countdownTimer > 0.0f)
				{
					outText << ceilf(countdownTimer);
					myFont->Draw(outText.str(), countdownTimerXLocation, countdownTimerYLocation);
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
			float speedometer = sqrt((momentum.x*momentum.x) + (momentum.z*momentum.z));	//finds the speed that the car is going using momentum in each direction to find the hypotenuse 
																								//of the vector the car is travelling 
			outText << ceilf(speedometer) << mphText;										//outputs the distance calculated using the formula above
			myFont->Draw(outText.str(), mphTextXLocation, mphTextYLocation);
			outText.str("");

			outText << ceilf(carDamage) << healthText;										//outputs the amount of health the car has to the ui
			myFont->Draw(outText.str(), healthTextXLocation, healthTextYLocation);
			outText.str("");

			outText << lapText << ceilf(lap) << numberOfLapsText;							//outputs the current lap and amount of laps to the ui
			myFont->Draw(outText.str(), lapTextXLocation, lapTextYLocation);
			outText.str("");

			goTimer -= frameTime;										

			if (goTimer > 0.0f)																//shows the text after the timer starts the race state
			{
				myFont->Draw( goText, goTextXLocation, goTextYLocation);
			}

			int MouseXMovement = myEngine->GetMouseMovementX();								//rotates the camera dummy depending on camera movement to allow the player to move the camera with the mouse 
			cameraDummy->RotateY(MouseXMovement);

			//save old position
			float carOldX = car->GetX();													//saves old positions of the car for collisions with objects
			float carOldZ = car->GetZ();

			//move the models
			// get the facing vector
			car->GetMatrix(&matrix[0][0]);													
			vector2D facingVector = { matrix[2][0], matrix[2][2] };
			// calculate thrust based on keyboard input 

			if (myEngine->KeyHeld(kAntiClockwiseTurnKey))
			{
				car->RotateY(-carRotationLimit);											//rotates the car anticlockwise to turn left
			}

			if (myEngine->KeyHeld(kClockwiseTurnKey))										//rotates the car clockwise to turn right 
			{
				car->RotateY(carRotationLimit);
			}

			if (myEngine->KeyHeld(kBoostKey) && onCooldown == false)														//starts a timer that goes down when the boost key is held and goes up when the boost key isnt
			{																												//moves the car by thrust factor * boost multiplier to make it faster 
				boostTimer -= frameTime;
				thrust = scalar((forwardsThrustFactor * boostMultiplier) * frameTime, facingVector);
			}
			else if (onCooldown == true)																					//if the car is on cooldown becasue they have overheated this slows the car for an amount of 
			{																												//time determined by the boostTimer and boostRechargeTimerLimit
				myFont->Draw( overheatedText, overheatedTextXLocation, overheatedTextYLocation);
				cooldownTimer -= frameTime;
				if (boostTimer >= 0.0f && boostTimer < boostRechargeTimerLimit)
				{
					boostTimer += frameTime;
				}
				if (myEngine->KeyHeld(kAccelerateKey))
				{
					thrust = scalar((forwardsThrustFactor/overheatedMultiplier) * frameTime, facingVector);					//this is the part that slows if the car has been overheated 
				}
				else if (myEngine->KeyHeld(kDecelerateKey))
				{
					thrust = scalar(reverseThrustFactor/overheatedMultiplier * frameTime, facingVector);
				}
				else
				{
					thrust = { 0.0f, 0.0f };
				}
				if (cooldownTimer < 0)
				{
					onCooldown = false;
				}

			}
			else
			{

				if (boostTimer >= 0.0f && boostTimer < boostRechargeTimerLimit)												//recharges the boost if the boost key isnt held
				{
					boostTimer += frameTime;
				}

				if (myEngine->KeyHeld(kAccelerateKey))																		//normal movement of the car as long as the boost button is held or is on cooldown 
				{
					thrust = scalar(forwardsThrustFactor * frameTime, facingVector);										
				}
				else if (myEngine->KeyHeld(kDecelerateKey))
				{
					thrust = scalar(reverseThrustFactor * frameTime, facingVector);
				}
				else
				{
					thrust = { 0.0f, 0.0f };
				}
			}

			if (onCooldown == false && boostTimer < overheatWarningTime)													//warns the player an amount of time before the car goes into overheat mode 
			{
				myFont->Draw(overheatingText, overheatingTextXLocation, overheatingTextYLocation);
			}

			if (boostTimer <= 0.0f)																							//sets the car to overheat if the limit on the timer has been reached 
			{
				cooldownTimer = cooldownLength;
				onCooldown = true;
				boostTimer = 0.0f;
			}
			// calculate drag based off of previous momentum

			drag = scalar(dragFactor*frameTime, momentum);

			// calculate momentum based on thrust, drag and previous momentum
			momentum = sum3(momentum, thrust, drag);
			// move the car depending on the new momentum
			car->Move(momentum.x*frameTime, 0.0f, momentum.z*frameTime);

			if (myEngine->KeyHeld(kCameraForwardKey))																		//camera movement controls
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
				myCamera->SetLocalPosition(camera3rdPersonXPosition, camera3rdPersonYPosition, camera3rdPersonZPosition);

			}

			if (myEngine->KeyHit(kFirstPersonCameraKey))
			{
				myCamera->SetLocalPosition(camera1stPersonXPosition, camera1stPersonYPosition, camera1stPersonZPosition);
			}

			//check for collisions
			//float dXPos, float dZPos, float dRad, float cXPos, float cZPos, float cRad

			for (int i = 0; i < kNumWalls; i++)
			{
				blockSide wallCollision = sphere2box(car->GetX(), car->GetZ(), carOldX, carOldZ, carRadius, wall[i]->GetX(), wall[i]->GetZ(), wallWidth, wallDepth);
				if (wallCollision == side || wallCollision == frontBack)
				{
					momentum.x = -momentum.x;											//reverses the momentum of the car to make it bounce if it has determines that an object has been hit 
					momentum.z = -momentum.z;											
					car->SetZ(carOldZ);													//sets the car to the old position to make sure that the collision doesnt keep happening 
					car->SetX(carOldX);
					carDamage -= damagePerHit;											//damages the car as it has hit something 
				}
				else if (wallCollision == noSide)
				{
				}
			}

			for (int i = 0; i < kNumCheckpoints; i++)
			{
				collision = sphere2sphere(car->GetX(), car->GetZ(), carRadius, checkpoint[i]->GetX() - checkpointLegDistence, checkpoint[i]->GetZ(), checkpointLegRadius);
				if (collision == true)
				{
					momentum.x = -momentum.x;											//does the same for each collision with an object 
					momentum.z = -momentum.z;
					car->SetZ(carOldZ);
					car->SetX(carOldX);
					carDamage -= damagePerHit;
				}
				collision = sphere2sphere(car->GetX(), car->GetZ(), carRadius, checkpoint[i]->GetX() + checkpointLegDistence, checkpoint[i]->GetZ(), checkpointLegRadius);
				if (collision == true)
				{
					momentum.x = -momentum.x;
					momentum.z = -momentum.z;
					car->SetZ(carOldZ);
					car->SetX(carOldX);
					carDamage -= damagePerHit;
				}
			}
			for (int i = 0; i < kNumTanks1; i++)
			{
				collision = sphere2sphere(car->GetX(), car->GetZ(), carRadius, tank1[i]->GetX(), tank1[i]->GetZ(), tankRadius);
				if (collision == true)
				{
					momentum.x = -momentum.x;
					momentum.z = -momentum.z;
					car->SetZ(carOldZ);
					car->SetX(carOldX);
					carDamage -= damagePerHit;
				}
			}

			for (int i = 0; i < kNumTanks2; i++)
			{
				collision = sphere2sphere(car->GetX(), car->GetZ(), carRadius, tank2[i]->GetX()-tankRotationHitboxOffset, tank2[i]->GetZ(), tankRadius);
				if (collision == true)
				{
					momentum.x = -momentum.x;
					momentum.z = -momentum.z;
					car->SetZ(carOldZ);
					car->SetX(carOldX);
					carDamage -= damagePerHit;
				}
			}

			if (carDamage <= 0)														//ends the game if the car reaches 0 health 
			{
				carDestroyed = true;
				currentStateG = finished;
			}

			break;
		}
		case finished:
		{
			if (carDestroyed == true)																			//gives different text depending on whether the race was finished or the car was destroyed 
			{
				myFont->Draw(totaledText, totaledTextXLocation, totaledTextYLocation);
			}
			else
			{
				myFont->Draw(raceCompleteText, raceCompleteTextXLocation, raceCompleteTextYLocation);
			}
			break;
		}
		}



		//start of switch currentStateC
		switch (currentStateC)
		{
		case firstCheckpoint:
		{
			// in the UI at the bottom display "stage 1 complete"
			myFont->Draw(stage1Text, stagesTextXLocation, stagesTextYLocation);									//shows the current stage, same for the start of each checkpoint 
			
			stageCompleteTimer -= frameTime;																	//timer for a stage complete popup
			if (stageCompleteTimer > 0.0f && lapStart == true)
			{
				outText << lapText << ceilf(lap) << numberOfLapsText;											//outputs what lap has been started as this is the start line 
				myFont->Draw(outText.str(), lapNumberPopupXLocation, lapNumberPopupYLocation);
				outText.str("");
				

			}

			collision = sphere2point(car->GetX(), car->GetZ(), carRadius, checkpoint[1]->GetX(), checkpoint[1]->GetZ());		//if the car has collided with the checkpoint moves to the next checkpoint
			if (collision == true)
			{
				stageCompleteTimer = stageCompletionTimerLength;
				currentStateC = secondCheckpoint;
			}
			//reslove collisions
			break;
		}
		case secondCheckpoint:
		{
			myFont->Draw(stage2Text, stagesTextXLocation, stagesTextYLocation);									//only difference is that it shows what stage has been completed instead of lap started 

			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f)
			{
			
				myFont->Draw(stage1CompleteText, stageCompleteTextXLocation, stageCompleteTextYLocation);
				
			}

			collision = sphere2point(car->GetX(), car->GetZ(), carRadius, checkpoint[2]->GetX(), checkpoint[2]->GetZ());
			if (collision == true)
			{
				stageCompleteTimer = stageCompletionTimerLength;												
				currentStateC = thirdCheckpoint;
			}
			break;
		}
		case thirdCheckpoint:
		{
			myFont->Draw(stage3Text, stagesTextXLocation, stagesTextYLocation);

			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f)
			{

				myFont->Draw(stage2CompleteText, stageCompleteTextXLocation, stageCompleteTextYLocation);

			}

			collision = sphere2point(car->GetX(), car->GetZ(), carRadius, checkpoint[3]->GetX(), checkpoint[3]->GetZ());
			if (collision == true)
			{
				stageCompleteTimer = stageCompletionTimerLength;
				currentStateC = fourthCheckpoint;
			}
			break;
		}
		case fourthCheckpoint:
		{
			myFont->Draw(stage4Text, stagesTextXLocation, stagesTextYLocation);
			
			stageCompleteTimer -= frameTime;
			if (stageCompleteTimer > 0.0f)
			{

				myFont->Draw(stage3CompleteText, stageCompleteTextXLocation, stageCompleteTextYLocation);

			}

			collision = sphere2point(car->GetX(), car->GetZ(), carRadius, checkpoint[0]->GetX(), checkpoint[0]->GetZ());
			if (collision == true)
			{
				if (lap == numberOfLaps)																		//if that was the final lap it ends the game 
				{
					currentStateC = finish;
				}
				else
				{
					lap += 1;																					//makes the lap counter go up by one as the finish line has been crossed 
					stageCompleteTimer = stageCompletionTimerLength;
					currentStateC = firstCheckpoint;
				}
			}

			break;
		}
		//add more game states here 
		case finish:
		{
			currentStateG = finished;									//stops movement 
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

bool sphere2sphere(float dumXPos, float dumZPos, float dumRad, float isleXPos, float isleZPos, float isleRad)		//used for collision with tanks and checkpoint legs
{																										
	float distX = isleXPos - dumXPos;
	float distZ = isleZPos - dumZPos;
	float distance = sqrt(distX*distX + distZ * distZ);

	return(distance < (dumRad + isleRad));
}

bool sphere2point(float dumXPos, float dumZPos, float dumRad, float checkXPos, float checkZPos)						//used for collision with checkpoints 
{
	float boxWidth = 5;
	float boxDepth = 2;

	float checkMinX = checkXPos - boxWidth;																			//creates a box for the car to collide with to transition to the next checkpoint 
	float checkMaxX = checkXPos + boxWidth;
	float checkMinZ = checkZPos - boxDepth;
	float checkMaxZ = checkZPos + boxDepth;

	if (dumXPos > checkMinX && dumXPos < checkMaxX && dumZPos >= checkMinZ && dumZPos <= checkMaxZ)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}


blockSide sphere2box(float d1XPos, float d1ZPos, float d1OldXPos, float d1OldZPos, float d1Rad, float wXPos, float wZPos, float wWidth, float wDepth)	
{
	float minX = wXPos - wWidth / 2 - d1Rad;			//creation of a box around walls to determine if the car dummies have collided by adding the radius of the car to the radius of the wall
	float maxX = wXPos + wWidth / 2 + d1Rad;
	float minZ = wZPos - wDepth / 2 - d1Rad;
	float maxZ = wZPos + wDepth / 2 + d1Rad;

	blockSide result = noSide;

	if (d1XPos > minX && d1XPos < maxX && d1ZPos > minZ && d1ZPos < maxZ)	//outputs which side the wall has hit to where the function has been called to change vectors appropriately
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



