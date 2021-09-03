#include "game.h"
namespace
{
	double deg2rad = acos(-1) / 180;
	double rad2deg = 1.0 / deg2rad;
	glm::mat4 PxMatToGlmMat(PxMat44 srcMat)
	{
		GLfloat valueBuffer[16];
		valueBuffer[0] = (GLfloat)srcMat.column0.x;
		valueBuffer[1] = (GLfloat)srcMat.column0.y;
		valueBuffer[2] = (GLfloat)srcMat.column0.z;
		valueBuffer[3] = (GLfloat)srcMat.column0.w;
		valueBuffer[4] = (GLfloat)srcMat.column1.x;
		valueBuffer[5] = (GLfloat)srcMat.column1.y;
		valueBuffer[6] = (GLfloat)srcMat.column1.z;
		valueBuffer[7] = (GLfloat)srcMat.column1.w;
		valueBuffer[8] = (GLfloat)srcMat.column2.x;
		valueBuffer[9] = (GLfloat)srcMat.column2.y;
		valueBuffer[10] = (GLfloat)srcMat.column2.z;
		valueBuffer[11] = (GLfloat)srcMat.column2.w;
		valueBuffer[12] = (GLfloat)srcMat.column3.x;
		valueBuffer[13] = (GLfloat)srcMat.column3.y;
		valueBuffer[14] = (GLfloat)srcMat.column3.z;
		valueBuffer[15] = (GLfloat)srcMat.column3.w;
		glm::mat4 matBuffer = glm::make_mat4(valueBuffer);
		return matBuffer;
	}
}

Game::Game(int raceTrackID, int raceCarID)
:mNbPlayers(1)
{
	//Initiallize Variables
	for (int i = 0; i < 256; i++)
		mKeyPress[i] = false;

	//Create FBX Loader And Set Terrian File Path
	bool supportVBO = GLEW_VERSION_1_5 ? true : false;
	char id_buffer[10];
	char* fbxFileName = new char[100];
	strcpy_s(fbxFileName, 100, "./model/racetrack/racetrack");
	_itoa_s(raceTrackID, id_buffer, 10, 10);
	strcat_s(fbxFileName, 100, id_buffer);
	strcat_s(fbxFileName, 100, ".fbx");
	mRaceTrackFBX	= new FbxLoader(fbxFileName, supportVBO);

	strcpy_s(mTerrianFilePath, 100, "./model/racetrack/racetrack");
	strcat_s(mTerrianFilePath, 100, id_buffer);
	strcat_s(mTerrianFilePath, 100, "_terrian.obj");

	fbxFileName = new char[100];
	strcpy_s(fbxFileName, 100, "./model/racecar/car");
	_itoa_s(raceCarID, id_buffer, 10, 10);
	strcat_s(fbxFileName, 100, id_buffer);
	strcat_s(fbxFileName, 100, ".fbx");
	mRaceCarFBX		= new FbxLoader(fbxFileName, supportVBO);
	
	fbxFileName = new char[100];
	strcpy_s(fbxFileName, 100, "./model/racecar/car");
	_itoa_s(raceCarID, id_buffer, 10, 10);
	strcat_s(fbxFileName, 100, id_buffer);
	strcat_s(fbxFileName, 100, "_tire.fbx");
	mRaceCarTireFBX = new FbxLoader(fbxFileName, supportVBO);

	PxF32 lSteerVsForwardSpeedData[2 * 8] =
	{
		0.0f,		0.75f,
		5.0f,		0.75f,
		30.0f,		0.125f,
		120.0f,		0.1f,
		PX_MAX_F32, PX_MAX_F32,
		PX_MAX_F32, PX_MAX_F32,
		PX_MAX_F32, PX_MAX_F32,
		PX_MAX_F32, PX_MAX_F32
	};
	mSteerVsForwardSpeedTable = new PxFixedSizeLookupTable<8>(lSteerVsForwardSpeedData, 4);
	return;
}
Game::Game(int raceTrackID, int raceCarID_1, int raceCarID_2)
:mNbPlayers(2)
{
	return;
}
Game::~Game(void)
{
	if (mRaceTrackFBX)
		delete mRaceTrackFBX;
	if (mRaceCarFBX)
		delete mRaceCarFBX;
	if (mSteerVsForwardSpeedTable)
		delete mSteerVsForwardSpeedTable;
	mRaceCar->getRigidDynamicActor()->release();
	mRaceCar->free();
	PX_RELEASE(mRaceTrack);
	PX_RELEASE(mBatchQuery);
	mVehicleSceneQueryData->free(mAllocator);
	PX_RELEASE(mFrictionPairs);
	PxCloseVehicleSDK();
	PX_RELEASE(mMaterial);
	PX_RELEASE(mCooking);
	PX_RELEASE(mScene);
	PX_RELEASE(mDispatcher);
	PX_RELEASE(mPhysics);
	PX_RELEASE(mFoundation);
	return;
}
bool Game::init(void)
{
	//Load FBX
	mRaceTrackFBX->loadFile();
	mRaceCarFBX->loadFile();
	mRaceCarTireFBX->loadFile();

	//Initiallize PhysX Basic Things
	{
		mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
		mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true);
		PxU32 numWorkers = 1;
		mDispatcher = PxDefaultCpuDispatcherCreate(numWorkers);
		PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
		sceneDesc.gravity = PxVec3(0.0f, -15.0f, 0.0f);
		sceneDesc.cpuDispatcher = mDispatcher;
		sceneDesc.filterShader = VehicleFilterShader;
		mScene = mPhysics->createScene(sceneDesc);
		mMaterial = mPhysics->createMaterial(0.65f, 0.5f, 0.1f);
		mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(PxTolerancesScale()));
	}
	
	//Initiallize PhysX Vehicle SDK
	{
		PxInitVehicleSDK(*mPhysics);
		PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
		PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
		mVehicleSceneQueryData = VehicleSceneQueryData::allocate(1, PX_MAX_NB_WHEELS, 1, 1, WheelSceneQueryPreFilterBlocking, NULL, mAllocator);
		mBatchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *mVehicleSceneQueryData, mScene);
		mFrictionPairs = createFrictionPairs(mMaterial);
	}
	
	//Create Race Track
	{
		//Load Race Track Information
		mCurrentRound = 0;
		mGoalRound = 3;
		{
			mGoalLineAreaVerticeMin = glm::vec3(129.0, 18.0, 6.0);
			mGoalLineAreaVerticeMax = glm::vec3(141.0, 28.0, 51.0);

			mCheckPoint1VerticeMin = glm::vec3(-41.0, 18.0, -57.0);
			mCheckPoint1VerticeMax = glm::vec3(4.5, 28.0, 28.0);

			mCheckPoint2VerticeMin = glm::vec3(-70.0, 55.0, -71.0);
			mCheckPoint2VerticeMax = glm::vec3(40.0, 65.0, 11.0);
		}
		

		PxU32 numVertices;
		PxU32 numTriangles;
		PxVec3* vertices = nullptr;
		PxU32* indices = nullptr;

		//Load Terrian Data
		FILE* fptr = nullptr;
		fopen_s(&fptr, mTerrianFilePath, "r");
		int vertice_index = 0, triangle_index = 0;
		char line_buffer[200];
		char *token, *next_token;
		char *inner_token, *inner_next_token;
		PxVec3 vec_buffer;
		while (fgets(line_buffer, sizeof(line_buffer), fptr) != NULL)
		{
			if (line_buffer[0] == '#' && line_buffer[1] == '!')
			{
				token = strtok_s(line_buffer, " \t\n", &next_token);
				for (int i = 0; i < 3; i++)
				{
					switch (i)
					{
					case 1:
						numVertices = atoi(token);
						break;
					case 2:
						numTriangles = atoi(token);
						break;
					default:
						;
					}
					token = strtok_s(NULL, " \t\n", &next_token);
				}
				vertices = new PxVec3[numVertices];
				indices = new PxU32[numTriangles * 3];
			}
			else if (line_buffer[0] == 'v' && line_buffer[1] == ' ')
			{
				token = strtok_s(line_buffer, " \t\n", &next_token);
				for (int i = 0; i < 4; i++)
				{
					switch (i)
					{
					case 1:
						vec_buffer.x = (float)(atof(token));
						break;
					case 2:
						vec_buffer.y = (float)(atof(token));
						break;
					case 3:
						vec_buffer.z = (float)(atof(token));
						break;
					default:
						;
					}
					token = strtok_s(NULL, " \t\n", &next_token);
				}
				vertices[vertice_index++] = vec_buffer;
			}
			else if (line_buffer[0] == 'f' && line_buffer[1] == ' ')
			{
				token = strtok_s(line_buffer, " \t\n", &next_token);
				int indice_buffer1, indice_buffer2, indice_buffer3;
				for (int i = 0; i < 4; i++)
				{
					inner_token = strtok_s(token, "/", &inner_next_token);
					switch (i)
					{
					case 1:
						indice_buffer1 = atoi(inner_token) - 1;
						break;
					case 2:
						indice_buffer2 = atoi(inner_token) - 1;
						break;
					case 3:
						indice_buffer3 = atoi(inner_token) - 1;
						break;
					default:
						;
					}
					token = strtok_s(NULL, " \t\n", &next_token);
				}
				indices[triangle_index++] = indice_buffer1;
				indices[triangle_index++] = indice_buffer2;
				indices[triangle_index++] = indice_buffer3;
			}
		}
		fclose(fptr);

		//Set Descriptor
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = numVertices;
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.points.data = vertices;
		meshDesc.triangles.count = numTriangles;
		meshDesc.triangles.stride = 3 * sizeof(PxU32);
		meshDesc.triangles.data = indices;

		//Create Terrian Triangle Mesh
		PxTriangleMesh* triMesh;
		PxU32 meshSize = 0;
		PxDefaultMemoryOutputStream outBuffer;
		PxTriangleMeshCookingResult::Enum result;//For Debug
		mCooking->cookTriangleMesh(meshDesc, outBuffer, &result);
		PxDefaultMemoryInputData stream(outBuffer.getData(), outBuffer.getSize());
		triMesh = mPhysics->createTriangleMesh(stream);
		meshSize = outBuffer.getSize();

		//Create Race Track Actor
		PxRigidStatic* raceTrackActor = mPhysics->createRigidStatic(PxTransform(PxIdentity));
		PxShape* raceTrackShape = mPhysics->createShape(PxTriangleMeshGeometry(triMesh), *mMaterial);
		PxFilterData raceTrackSimFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_OBSTACLE_AGAINST, 0, 0);
		PxFilterData raceTrackQryFilterData;
		setupDrivableSurface(raceTrackQryFilterData);
		raceTrackShape->setSimulationFilterData(raceTrackSimFilterData);
		raceTrackShape->setQueryFilterData(raceTrackQryFilterData);
		raceTrackActor->attachShape(*raceTrackShape);
		mScene->addActor(*raceTrackActor);

		//Release Data
		raceTrackShape->release();
		triMesh->release();
		delete vertices;
		delete indices;
	}

	//Create Race Car
	{
		mRaceCarDirection;
		mRaceCarUp;
		mRaceCarRight;
		mRaceCarPosition = glm::vec4(135.0f, 19.5f, 29.2, 1.0f);
		VehicleDesc vehicleDesc = initVehicleDesc();
		mRaceCar = createVehicle4W(vehicleDesc, mPhysics, mCooking);
		PxMat44 startPose;
		startPose.column0 = PxVec4(0.0f, 0.0f, 1.0f, 0.0f);
		startPose.column1 = PxVec4(0.0f, 1.0f, 0.0f, 0.0f);
		startPose.column2 = PxVec4(-1.0f, 0.0f, 0.0f, 0.0f);
		startPose.column3 = PxVec4(mRaceCarPosition.x, mRaceCarPosition.y, mRaceCarPosition.z, 1.0f);
		mRaceCar->getRigidDynamicActor()->setGlobalPose(PxTransform(startPose));
		mRaceCarSavePose = mRaceCar->getRigidDynamicActor()->getGlobalPose();
		mScene->addActor(*mRaceCar->getRigidDynamicActor());
		mRaceCar->setToRestState();
		mRaceCar->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		mRaceCar->mDriveDynData.setUseAutoGears(true);
	}

	mCountDown = 5;
	mGameReadyTime = clock();
	mGameStatus = eREADY;
	return true;
}
void Game::render(void)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glm::vec4 lVecBuffer;

	//Compute Camera Position
	mCameraPosition = mRaceCarPosition - mRaceCarDirection * glm::vec4(8.0f) + glm::vec4(0.0f, 5.0f, 0.0f, 0.0f) + mRaceCarRight * glm::vec4(mRaceCar->computeSidewaysSpeed() / 3.0);
	
	//Set Camera
	gluLookAt(mCameraPosition.x, mCameraPosition.y, mCameraPosition.z, mRaceCarPosition.x, mRaceCarPosition.y, mRaceCarPosition.z, 0.0f, 1.0f, 0.0f);
	
	//Draw
	glPushMatrix();
		//Race Track
		glPushMatrix();
			mRaceTrackFBX->draw();
		glPopMatrix();

		//Race Car
		glPushMatrix();
			lVecBuffer = mRaceCarUp * glm::vec4(-1.3f);
			glTranslatef(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z);
			glMultMatrixf(glm::value_ptr(PxMatToGlmMat(PxMat44(mRaceCar->getRigidDynamicActor()->getGlobalPose()))));
			glScalef(-1.0f, 1.0f, 1.0f);
			mRaceCarFBX->draw();
		glPopMatrix();

		//Race Car Tire
		PxShape* shapes[4];
		mRaceCar->getRigidDynamicActor()->getShapes(shapes, 4);
		for (PxU32 i = 0; i < 4; i++)
		{
			PxMat44 lTempSavePose;
			PxRigidActor* actor = shapes[i]->getActor();
			lTempSavePose = PxShapeExt::getGlobalPose(*shapes[i], *actor);

			glPushMatrix();
				//Align Front Tire
				if (i == 0 || i == 1)
				{
					lVecBuffer = mRaceCarDirection * glm::vec4(0.075f);
					glTranslatef(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z);
				}

				//Align Back Tire
				if (i == 2 || i == 3)
				{
					lVecBuffer = mRaceCarDirection * glm::vec4(0.275f);
					glTranslatef(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z);
				}

				//Align Right Tire
				if (i == 0 || i == 2)
				{
					lVecBuffer = mRaceCarRight * glm::vec4(-0.03f);
					glTranslatef(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z);
				}

				//Align Left Tire
				if (i == 1 || i == 3)
				{
					lVecBuffer = mRaceCarRight * glm::vec4(0.03f);
					glTranslatef(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z);
				}
				glMultMatrixf(glm::value_ptr(PxMatToGlmMat(lTempSavePose)));

				//Scale Back Tire To Mach
				if (i == 2 || i == 3)
					glScalef(1.143f, 1.143f, 1.143f);
				//Flip Tire To Right Side
				if(i == 1 || i == 3)
					glScalef(-1.0f, 1.0f, -1.0f);
				mRaceCarTireFBX->draw();
			glPopMatrix();
		}

	glPopMatrix();
	return;
}
void Game::stepPhysics(PxF32 dt)
{
	if (mGameStatus == eREADY)
	{
		if (mCountDown - (clock() - mGameReadyTime) / 1000 - 1 < 0)
		{
			mGameStatus = eGAMING;
			mGameStartTime = clock();
		}
	}

	glm::vec4 lVecBuffer;
	PxMat44 lTempSavePose;

	//Reset Control
	releaseAllControls();

	//Process Keyboard Input
	{
		//Direction
		if (mKeyPress['W'] || mKeyPress['w'])
		{
			mRaceCar->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
			mVehicleInputData.setDigitalAccel(true);
		}
		if (mKeyPress['A'] || mKeyPress['a'])
		{
			mVehicleInputData.setDigitalSteerRight(true);
		}
		if (mKeyPress['S'] || mKeyPress['s'])
		{
			mRaceCar->mDriveDynData.forceGearChange(PxVehicleGearsData::eREVERSE);
			mVehicleInputData.setDigitalAccel(true);
		}
		if (mKeyPress['D'] || mKeyPress['d'])
		{
			mVehicleInputData.setDigitalSteerLeft(true);
		}

		//Handbrake
		if (mKeyPress['L'] || mKeyPress['l'])
		{
			mVehicleInputData.setDigitalHandbrake(true);
		}

		//Reset Race Car To Saved Point
		if (mKeyPress['R'] || mKeyPress['r'] || mRaceCarPosition.y < 0.0)
		{
			mRaceCar->getRigidDynamicActor()->setGlobalPose(PxTransform(mRaceCarSavePose));
			mRaceCar->setToRestState();
		}

		//Cheating
		if (mKeyPress['J'] || mKeyPress['j'])
		{
			PxRigidDynamic *actor = mRaceCar->getRigidDynamicActor();
			lTempSavePose = actor->getGlobalPose();
			lVecBuffer = mRaceCarPosition + mRaceCarDirection * glm::vec4(3.0f);
			lTempSavePose.column3 = PxVec4(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z, 1.0f);
			mRaceCar->getRigidDynamicActor()->setGlobalPose(PxTransform(lTempSavePose));
		}
		if (mKeyPress['K'] || mKeyPress['k'])
		{
			PxRigidDynamic *actor = mRaceCar->getRigidDynamicActor();
			PxMat44 lTempSavePose = actor->getGlobalPose();
			lVecBuffer = mRaceCarPosition - mRaceCarDirection * glm::vec4(3.0f);
			lTempSavePose.column3 = PxVec4(lVecBuffer.x, lVecBuffer.y, lVecBuffer.z, 1.0f);
			mRaceCar->getRigidDynamicActor()->setGlobalPose(PxTransform(lTempSavePose));
		}

		/*if (mKeyPress['O'] || mKeyPress['o'])
		{//加速帶 氮氣
		PxVehicleDriveDynData* driveDynData = &mRaceCar->mDriveDynData;
		driveDynData->setEngineRotationSpeed(2000);
		}*/

		/*if (mKeyPress['I'] || mKeyPress['i'])
		{//小噴 跳台
		PxRigidDynamic *actor = mRaceCar->getRigidDynamicActor();
		//PxVec3 forceBuffer(mRaceCarDirection.x, mRaceCarDirection.y, mRaceCarDirection.z);
		PxVec3 forceBuffer(0, 10, 0);
		actor->addForce(forceBuffer * 50, PxForceMode::eACCELERATION);
		}*/
	}

	//Update the control inputs for the vehicle.
	PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(mKeySmoothingData, *mSteerVsForwardSpeedTable, mVehicleInputData, dt, mIsVehicleInAir, *mRaceCar);

	//Raycasts.
	PxVehicleWheels* vehicles[1] = { mRaceCar };
	PxRaycastQueryResult* raycastResults = mVehicleSceneQueryData->getRaycastQueryResultBuffer(0);
	const PxU32 raycastResultsSize = mVehicleSceneQueryData->getQueryResultBufferSize();
	PxVehicleSuspensionRaycasts(mBatchQuery, 1, vehicles, raycastResultsSize, raycastResults);

	//Vehicle update.
	const PxVec3 grav = mScene->getGravity();
	PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
	PxVehicleWheelQueryResult vehicleQueryResults[1] = { { wheelQueryResults, mRaceCar->mWheelsSimData.getNbWheels() } };
	PxVehicleUpdates(dt, grav, *mFrictionPairs, 1, vehicles, vehicleQueryResults);

	//Work out if the vehicle is in the air.
	mIsVehicleInAir = mRaceCar->getRigidDynamicActor()->isSleeping() ? false : PxVehicleIsInAir(vehicleQueryResults[0]);
	
	//Save Point
	static int saveCount = (int)(3.0 / dt);
	if (!mIsVehicleInAir && (--saveCount) <= 0)
	{
		lTempSavePose = mRaceCar->getRigidDynamicActor()->getGlobalPose();
		if (lTempSavePose.column1.y < lTempSavePose.column1.x || lTempSavePose.column1.y < lTempSavePose.column1.z)
			++saveCount;
		else
		{
			cout << "Save Position ( " << mRaceCarPosition.x << ", " << mRaceCarPosition.y << ", " << mRaceCarPosition.z << " )" << endl;
			mRaceCarSavePose = lTempSavePose;
			saveCount = (int)(3.0 / dt);
		}
	}
		
	mScene->simulate(dt);
	mScene->fetchResults(true);

	//Refresh Race Car Position And Direction
	{
		PxRigidDynamic *actor = mRaceCar->getRigidDynamicActor();
		PxMat44 actorPose(actor->getGlobalPose());
		PxVec3 actorPosition = actorPose.getPosition();
		mRaceCarPosition = glm::vec4(actorPosition.x, actorPosition.y, actorPosition.z, 1.0f);
		PxVec3 actorAxisX = actorPose.getBasis(0);
		PxVec3 actorAxisY = actorPose.getBasis(1);
		PxVec3 actorAxisZ = actorPose.getBasis(2);
		mRaceCarDirection = glm::vec4(actorAxisZ.x, actorAxisZ.y, actorAxisZ.z, 0.0f);
		mRaceCarUp = glm::vec4(actorAxisY.x, actorAxisY.y, actorAxisY.z, 0.0f);
		mRaceCarRight = glm::vec4(actorAxisX.x, actorAxisX.y, actorAxisX.z, 0.0f) * glm::vec4(-1.0f);
	}
	

	//Update Check Point
	{
		//Goal Check
		if (isCarInArea(mGoalLineAreaVerticeMin, mGoalLineAreaVerticeMax, glm::vec3(mRaceCarPosition)))
		{
			if (mCheckPoint1Flag && mCheckPoint2Flag)
			{
				mCurrentRound++;
				mCheckPoint1Flag = false;
				mCheckPoint2Flag = false;
			}
		}

		//Check Point 1
		if (isCarInArea(mCheckPoint1VerticeMin, mCheckPoint1VerticeMax, glm::vec3(mRaceCarPosition)))
		{
			mCheckPoint1Flag = true;
			mCheckPoint2Flag = false;
		}
		//Check Point 2
		if (mCheckPoint1Flag && isCarInArea(mCheckPoint2VerticeMin, mCheckPoint2VerticeMax, glm::vec3(mRaceCarPosition)))
		{
			mCheckPoint2Flag = true;
		}
	}

	//End Game Check
	if (mGameStatus == eGAMING && mCurrentRound >= mGoalRound)
	{
		mGameStatus = eOVER;
		mGameRecordTime = (int)(clock() - mGameStartTime);
		//Release all keyboard input
		for (int i = 0; i < 256; i++)
			mKeyPress[i] = false;
	}

	return;
}
void Game::keyboard(unsigned char key)
{
	if(mGameStatus == eGAMING)
		mKeyPress[key] = true;
}
void Game::keyboardUp(unsigned char key)
{
	mKeyPress[key] = false;
}

//Private Function
VehicleDesc Game::initVehicleDesc(void)
{
	//Set up the chassis mass, dimensions, moment of inertia, and center of mass offset.
	//The moment of inertia is just the moment of inertia of a cuboid but modified for easier steering.
	//Center of mass offset is 0.65m above the base of the chassis and 0.25m towards the front.
	const PxF32 chassisMass = 1500.0f;
	const PxVec3 chassisDims(2.1f, 1.2f, 4.0f);
	const PxVec3 chassisMOI
	((chassisDims.y*chassisDims.y + chassisDims.z*chassisDims.z)*chassisMass / 12.0f,
		(chassisDims.x*chassisDims.x + chassisDims.z*chassisDims.z)*0.8f*chassisMass / 12.0f,
		(chassisDims.x*chassisDims.x + chassisDims.y*chassisDims.y)*chassisMass / 12.0f);
	const PxVec3 chassisCMOffset(0.0f, -chassisDims.y*0.5f + 0.65f, 0.25f);

	//Set up the wheel mass, radius, width, moment of inertia, and number of wheels.
	//Moment of inertia is just the moment of inertia of a cylinder.
	const PxF32 wheelMass = 20.0f;
	const PxF32 wheelRadius = 0.35f;
	const PxF32 wheelWidth = 0.4f;
	const PxF32 wheelMOI = 0.5f * wheelMass * wheelRadius * wheelRadius;
	const PxU32 nbWheels = 4;

	VehicleDesc vehicleDesc;

	vehicleDesc.chassisMass = chassisMass;
	vehicleDesc.chassisDims = chassisDims;
	vehicleDesc.chassisMOI = chassisMOI;
	vehicleDesc.chassisCMOffset = chassisCMOffset;
	vehicleDesc.chassisMaterial = mMaterial;
	vehicleDesc.chassisSimFilterData = PxFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS_AGAINST, 0, 0);

	vehicleDesc.wheelMass = wheelMass;
	vehicleDesc.wheelRadius = wheelRadius;
	vehicleDesc.wheelWidth = wheelWidth;
	vehicleDesc.wheelMOI = wheelMOI;
	vehicleDesc.numWheels = nbWheels;
	vehicleDesc.wheelMaterial = mMaterial;
	vehicleDesc.chassisSimFilterData = PxFilterData(COLLISION_FLAG_WHEEL, COLLISION_FLAG_WHEEL_AGAINST, 0, 0);

	return vehicleDesc;
}
void Game::releaseAllControls(void)
{
	mVehicleInputData.setDigitalAccel(false);
	mVehicleInputData.setDigitalSteerLeft(false);
	mVehicleInputData.setDigitalSteerRight(false);
	mVehicleInputData.setDigitalBrake(false);
	mVehicleInputData.setDigitalHandbrake(false);
	return;
}
bool Game::isCarInArea(glm::vec3 vMin, glm::vec3 vMax, glm::vec3 carPosition)
{
	if (carPosition.x < vMin.x || carPosition.x > vMax.x)
		return false;
	if (carPosition.y < vMin.y || carPosition.y > vMax.y)
		return false;
	if (carPosition.z < vMin.z || carPosition.z > vMax.z)
		return false;
	return true;
}