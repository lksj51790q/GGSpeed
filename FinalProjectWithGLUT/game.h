#include <Windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

#include "fbx/FbxLoader.h"


#include <PxPhysicsAPI.h>
#include "vehicle/PxVehicleUtil.h"
#include "physx/SnippetVehicleSceneQuery.h"
#include "physx/SnippetVehicleFilterShader.h"
#include "physx/SnippetVehicleTireFriction.h"
#include "physx/SnippetVehicleCreate.h"
//#include "snippetrender/SnippetRender.h"
//#include "snippetrender/SnippetCamera.h"

#include <vector>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <iostream>

#define AREA_DETECT_HEIGHT 10.0
#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace std;
using namespace physx;
using namespace snippetvehicle;

class Game
{
public:
	enum Status {
		eREADY,
		eGAMING,
		eOVER
	};
	Game(int raceTrackID, int raceCarID);
	Game(int raceTrackID, int raceCarID_1, int raceCarID_2);
	~Game(void);

	bool init(void);
	int getNbPlayers() const { return mNbPlayers; };
	Status getStatus() const { return mGameStatus; };
	int getCountDown() const { return mCountDown - (clock() - mGameReadyTime) / 1000; };
	int getCurrentRound() const { return mCurrentRound; };
	int getGoalRound() const { return mGoalRound; };
	float getRaceCarForwardSpeed() const { return (float)mRaceCar->computeForwardSpeed(); };
	float getRaceCarSidewaysSpeed() const { return (float)mRaceCar->computeSidewaysSpeed(); };
	int getGameTime() const { return (int)(clock() - mGameStartTime); };
	int getRecordTime() const { return mGameRecordTime; };

	void render(void);
	void stepPhysics(PxF32 dt);
	void keyboard(unsigned char key);
	void keyboardUp(unsigned char key);

private:
	//Variables
	int			mNbPlayers;
	bool		mKeyPress[256];
	clock_t		mGameReadyTime;
	int			mCountDown;
	clock_t		mGameStartTime;
	int			mGameRecordTime;
	Status		mGameStatus = eREADY;

	FbxLoader*	mRaceTrackFBX	= nullptr;
	FbxLoader*	mRaceCarFBX		= nullptr;
	FbxLoader*	mRaceCarTireFBX	= nullptr;
	char		mTerrianFilePath[100];

	glm::vec4	mRaceCarInitDirection	= glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	glm::vec4	mRaceCarInitUp			= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	glm::vec4	mRaceCarPosition;
	glm::vec4	mRaceCarDirection;
	glm::vec4	mRaceCarUp;
	glm::vec4	mRaceCarRight;
	glm::vec4	mCameraPosition;
	PxMat44		mRaceCarSavePose;

	//Area detect
	int mCurrentRound;
	int mGoalRound;
	glm::vec3	mGoalLineAreaVerticeMin;
	glm::vec3	mGoalLineAreaVerticeMax;
	glm::vec3	mCheckPoint1VerticeMin;
	glm::vec3	mCheckPoint1VerticeMax;
	bool		mCheckPoint1Flag = false;
	glm::vec3	mCheckPoint2VerticeMin;
	glm::vec3	mCheckPoint2VerticeMax;
	bool		mCheckPoint2Flag = false;

	PxDefaultAllocator								mAllocator;
	PxDefaultErrorCallback							mErrorCallback;
	PxFoundation*									mFoundation				= nullptr;
	PxPhysics*										mPhysics				= nullptr;
	PxDefaultCpuDispatcher*							mDispatcher				= nullptr;
	PxScene*										mScene					= nullptr;
	PxCooking*										mCooking				= nullptr;
	PxMaterial*										mMaterial				= nullptr;
	VehicleSceneQueryData*							mVehicleSceneQueryData	= nullptr;
	PxBatchQuery*									mBatchQuery				= nullptr;
	PxVehicleDrivableSurfaceToTireFrictionPairs*	mFrictionPairs			= nullptr;
	PxRigidStatic*									mRaceTrack				= nullptr;
	PxVehicleDrive4W*								mRaceCar				= nullptr;
	bool											mIsVehicleInAir			= true;

	PxFixedSizeLookupTable<8> *mSteerVsForwardSpeedTable = nullptr;
	PxVehicleKeySmoothingData mKeySmoothingData =
	{
		{
			6.0f,	//rise rate eANALOG_INPUT_ACCEL
			6.0f,	//rise rate eANALOG_INPUT_BRAKE		
			6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE	
			2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT
			2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT
		},
		{
			10.0f,	//fall rate eANALOG_INPUT_ACCEL
			10.0f,	//fall rate eANALOG_INPUT_BRAKE		
			10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE	
			5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
			5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
		}
	};
	PxVehicleDrive4WRawInputData mVehicleInputData;

	//Function
	VehicleDesc initVehicleDesc(void);
	void releaseAllControls(void);
	bool isCarInArea(glm::vec3 vMin, glm::vec3 vMax, glm::vec3 carPosition);
};