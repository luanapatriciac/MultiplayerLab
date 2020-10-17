#include <CarLogic.h>
#include <Utility.h>
#include <iostream>
#include <ProjectileLogic.h>
#include <PickUp.h>
#include "ResourceHolder.h"
#include "NetworkCommon.h"
#include <math.h>


CarLogic::CarLogic() :
	mHP(1),
	mHpMax(1),
	mDrifting(false),
	mForward(true),
	mPrevDriftingSign(0),
	mCrash(false),
	mAction(CarAction::ShootBullet),
	mLaunchedMissile(false),
	mMissileAmmo(5),
	mInputs({ false, false, false, false, false, false, false }),
	Entity(sf::Vector2f(0, 0), sf::RectangleShape(sf::Vector2f(0, 0)))
{
	mType = Type::CarType;
}

CarLogic::CarLogic(int hp, sf::Vector2f pos, sf::RectangleShape rect) :
	mHP(hp),
	mHpMax(hp),
	mKeyBindings(nullptr),
	mShootDelay(sf::seconds(0.1)),
	mCrash(false),
	mAction(CarAction::ShootBullet),
	mLaunchedMissile(false),
	mMissileAmmo(5),
	mInputs({ false, false, false, false, false, false, false }),
	mTrajectory(),
	Entity(pos, rect)
{
	mType = Type::CarType;
	mCarDirection = sf::Vector2f(1, 0);

	mType = Type::CarType;
}

CarLogic::CarLogic(int hp, sf::Vector2f pos, sf::RectangleShape rect, KeyBinding* keys) :
	mHP(hp),
	mHpMax(hp),
	mKeyBindings(keys),
	mShootDelay(sf::seconds(0.1)),
	mCrash(false),
	mAction(CarAction::ShootBullet),
	mLaunchedMissile(false),
	mMissileAmmo(5),
	mInputs({ false, false, false, false, false, false, false }),
	Entity(pos, rect)
{
	mType = Type::CarType;
	mCarDirection = sf::Vector2f(1, 0);
}

void CarLogic::update(sf::Time dt, std::vector<Entity*> entities, std::vector<Entity*>& newEntities, std::set<Pair>& pairs)
{
	if (mCurrentShootDelay > sf::Time::Zero) mCurrentShootDelay -= dt;

	if (!mTrajectory.empty()) //maybe skip usual computation when we use dead reckoning?
	{
		stepUpDeadReckoning();
	}
	else
	{
		getInput();
		useInputs(dt, newEntities);
	}

	Entity::update(dt, entities, newEntities, pairs);
}

void CarLogic::serverUpdate(sf::Time serverTime, sf::Time dt, std::vector<Entity*> entities, std::vector<Entity*>& newEntities, std::set<Pair>& pairs)
{
	if (mCurrentShootDelay > sf::Time::Zero) mCurrentShootDelay -= dt;

	getInput(serverTime);
	useInputs(dt, newEntities);

	Entity::update(dt, entities, newEntities, pairs);

	mInputs = { false, false, false, false, false, false, false };
}

void CarLogic::getInput()
{
	if (mKeyBindings != nullptr)
	{
		mInputs.left = sf::Keyboard::isKeyPressed(mKeyBindings->getAssignedKey(PlayerAction::TurnLeft));
		mInputs.right = sf::Keyboard::isKeyPressed(mKeyBindings->getAssignedKey(PlayerAction::TurnRight));
		mInputs.up = sf::Keyboard::isKeyPressed(mKeyBindings->getAssignedKey(PlayerAction::Accelerate));
		mInputs.down = sf::Keyboard::isKeyPressed(mKeyBindings->getAssignedKey(PlayerAction::Brake));
		mInputs.action = sf::Keyboard::isKeyPressed(mKeyBindings->getAssignedKey(PlayerAction::DoAction));
	}
}

void CarLogic::getInput(sf::Time serverTime)
{
	std::map<sf::Time, Inputs>::iterator low;
	low = mServerInputs.lower_bound(serverTime);
	if (low == mServerInputs.begin())
	{
		//serverTime is lower than every input timestamp
		mInputs = { false, false, false, false, false, false, false };
	}
	else
	{
		--low;
		mInputs = low->second;
	}
}

inline CarLogic::CarAction operator++(CarLogic::CarAction& x)
{
	return x = (CarLogic::CarAction)(((int)(x)+1));
}

void CarLogic::useInputs(sf::Time dt, std::vector<Entity*>& newEntities)
{
	float l = length(mVelocity);

	//events handling
	if (mInputs.changeActionEvent)
	{
		++mAction;
		if (mAction == CarAction::ActionCount) mAction = (CarAction)0;
	}
	else if (mInputs.doActionEvent && needsEventInput())
	{
		switch (mAction)
		{
		case CarLogic::CarAction::LaunchMissile:
		{
			if (!mLaunchedMissile && mMissileAmmo > 0) mLaunchedMissile = true;
			break;
		}
		default:
			break;
		}
	}

	//realtime handling
	float angle = 0;
	float angleSign = 0;
	if (mInputs.left && l > 50)
	{
		angle += M_PI / 3;
		angleSign += 1;
	}
	if (mInputs.right && l > 50)
	{
		angle -= M_PI / 3;
		angleSign -= 1;
	}

	float accel = 0;
	bool driftBrake = false;
	if (mInputs.up)
	{
		float f = 1;
		if (!mForward) f = 10;
		accel += f * mCarAcceleration;
	}
	if (mInputs.down)
	{
		if (mForward && l > mDriftTheshold && angleSign != 0) driftBrake = true;
		else
		{
			float f = 1;
			if (mForward) f = 10;
			accel -= f * mCarAcceleration;
		}
	}
	if (accel == 0 && l > 200)
	{
		accel = (l * l + 2 * l) * mDrag;
		if (mForward) accel *= -1;
	}
	else if (accel == 0)
	{
		mVelocity = sf::Vector2f(0, 0);
	}

	float tangAccel = accel * cos(angle);
	float radAccel = accel * sin(angle);
	sf::Vector2f tangAccelVector = tangAccel * mCarDirection;
	mVelocity += tangAccelVector * dt.asSeconds();
	l = length(mVelocity);
	if (mForward && l > mCarMaxSpeed)
	{
		mVelocity *= mCarMaxSpeed / l;
	}
	else if (!mForward && l > mCarBackwardsMaxSpeed)
	{
		mVelocity *= mCarBackwardsMaxSpeed / l;
	}

	bool prevDrifting = mDrifting;
	mDrifting = mForward && l > mDriftTheshold && angleSign != 0 && driftBrake;

	float theta = sqrt(abs(radAccel) / mTurnRadius) * dt.asSeconds();
	mForward = dotProduct(mVelocity, mCarDirection) >= 0;
	if (!mForward)
	{
		mVelocity = rotate(mVelocity, -theta * angleSign);
		mCarDirection = rotate(mCarDirection, -theta * angleSign);
	}
	else
	{
		mVelocity = rotate(mVelocity, theta * angleSign);
		mCarDirection = rotate(mCarDirection, theta * angleSign);
	}

	if (prevDrifting && !mDrifting)
	{
		mCarDirection = rotate(mCarDirection, mPrevDriftingSign * mDriftAngle);
		mVelocity = rotate(mVelocity, mPrevDriftingSign * mDriftAngle);
	}
	mPrevDriftingSign = angleSign;

	float carAngle = 0;
	if (mCarDirection.x != 0) carAngle = -atan2(mCarDirection.y, mCarDirection.x);
	if (mCarDirection.x == 0 && mCarDirection.y != 0) carAngle = M_PI_2 * mCarDirection.y / abs(mCarDirection.y);
	if (mDrifting) carAngle += angleSign * mDriftAngle;
	mRotation = -carAngle * 180.0 / M_PI;

	sf::Vector2f projDir = mCarDirection;
	if (mDrifting) projDir = rotate(projDir, angleSign * mDriftAngle);

	if (mAction == CarAction::LaunchMissile && mLaunchedMissile)
	{
		mLaunchedMissile = false;
		mMissileAmmo--;

		ProjectileLogic* proj = new ProjectileLogic(5, sf::seconds(10), 400, 400, mPosition + 25.f * projDir, projDir, sf::RectangleShape(sf::Vector2f(30, 10)), this);
		newEntities.push_back(proj);
	}

	if (mInputs.action && !needsEventInput()) //&& mCurrentShootDelay <= sf::Time::Zero)
	{
		switch (mAction)
		{
		case CarLogic::CarAction::ShootBullet:
		{
			if (mCurrentShootDelay <= sf::Time::Zero)
			{
				mCurrentShootDelay = mShootDelay;

				ProjectileLogic* proj = new ProjectileLogic(1, sf::seconds(1), 1500, mPosition + 25.f * projDir, projDir, sf::RectangleShape(sf::Vector2f(5, 5)), this);
				newEntities.push_back(proj);
			}
			break;
		}
		default:
			break;
		}
	}
}

bool CarLogic::handleEvent(const sf::Event& event)
{
	if (mKeyBindings != nullptr)
	{
		mInputs.changeActionEvent = mInputs.changeActionEvent || event.type == sf::Event::KeyPressed && event.key.code == mKeyBindings->getAssignedKey(PlayerAction::ChangeAction);
		mInputs.doActionEvent = mInputs.doActionEvent || event.type == sf::Event::KeyPressed && event.key.code == mKeyBindings->getAssignedKey(PlayerAction::DoAction);
	}

	return true;
}

bool CarLogic::needsEventInput()
{
	bool needs = false;
	switch (mAction)
	{
	case CarAction::ShootBullet:
	{
		needs = false;
		break;
	}
	case CarAction::LaunchMissile:
	{
		needs = true;
		break;
	}
	case CarAction::ToggleMap:
	{
		needs = true;
		break;
	}
	default:
		break;
	}

	return needs;
}

void CarLogic::cleanUp(sf::Vector2f worldSize, sf::Time dt)
{
	if (mPosition.x > worldSize.x || mPosition.x < 0) mPosition.x -= mVelocity.x * dt.asSeconds();
	if (mPosition.y > worldSize.y || mPosition.y < 0) mPosition.y -= mVelocity.y * dt.asSeconds();

	mPrevCollidedWith = mCollidedWith;
	mCollidedWith.clear();
}

void CarLogic::crash(sf::Vector2f otherVelocity)
{
	mCrash = true;
	//mVelocity = sf::Vector2f(0, 0);
	mVelocity = otherVelocity;
}

void CarLogic::damage(int points)
{
	mHP -= points;
	std::cout << "took " << points << " dmg" << std::endl;
	if (mHP <= 0) mToRemove = true;
}

void CarLogic::repair(int points)
{
	mHP += points;
	if (mHP > mHpMax) mHP = mHpMax;
}

void CarLogic::addMissileAmmo(int ammo)
{
	mMissileAmmo += ammo;
}

void CarLogic::onCollision(Entity* other)
{
	switch (other->getType())
	{
	case Type::CarType:
	{
		Car* otherCar = dynamic_cast<Car*>(other);
		mCollidedWith.push_back(other);

		bool collision = true;
		for (auto& ent : mPrevCollidedWith)
		{
			if (ent == other)
			{
				collision = false;
				break;
			}
		}

		if (collision)
		{
			std::cout << "collision" << std::endl;
			damage(2);
			otherCar->damage(2);
		}

		sf::Vector2f prevVelocity = mVelocity;
		//crash(other->getVelocity());
		//otherCar->crash(prevVelocity);

		/*setVelocity(0.8f * otherCar->getVelocity());
		otherCar->setVelocity(0.8f * prevVelocity);*/
		mVelocity = sf::Vector2f(0, 0);
		other->setVelocity(sf::Vector2f(0, 0));

		break;
	}

	case Type::ProjectileType:
	{
		ProjectileLogic* otherProj = dynamic_cast<ProjectileLogic*>(other);
		if (otherProj->getCar() == this)
		{
			break;
		}

		damage(otherProj->getDamage());
		other->remove();
		break;
	}

	case Type::PickUpType:
	{
		PickUp* otherPickup = dynamic_cast<PickUp*>(other);
		otherPickup->onCollision(this);
	}
	default:
		break;
	}
}

sf::Vector2f CarLogic::getCarDirection()
{
	return mCarDirection;
}

std::string CarLogic::getActionText()
{
	std::string res = "null";
	switch (mAction)
	{
	case CarAction::ShootBullet:
	{
		res = "Shoot Bullets";
		break;
	}
	case CarAction::LaunchMissile:
	{
		res = "Launch Missile (x" + std::to_string(mMissileAmmo) + ")";
		break;
	}
	case CarAction::ToggleMap:
	{
		res = "Toggle Map";
		break;
	}
	default:
		break;
	}

	return res;
}

float CarLogic::getSpeedRatio()
{
	return length(mVelocity) / mCarMaxSpeed;
}

Inputs CarLogic::getSavedInputs()
{
	return mInputs;
}

void CarLogic::setInputs(Inputs inputs)
{
	mInputs = inputs;
}

void CarLogic::computeDeadReckoning(sf::Vector2f newPosition, sf::Vector2f newVelocity, sf::Vector2f newCarDirection)
{
	int numberOfSteps;
	numberOfSteps = floor(TimePerFrame / TimePerTick);
	mTrajectory.empty();

	for (int i = 0; i < numberOfSteps; i++)
	{
		mTrajectory.push({ mPosition + ((1.f / (numberOfSteps - i)) * (newPosition - mPosition)), mVelocity + ((1.f / (numberOfSteps - i)) * (newVelocity - mVelocity)), mCarDirection + ((1.f / (numberOfSteps - i)) * (newCarDirection - mCarDirection)) });
	}
}

void CarLogic::stepUpDeadReckoning()
{
	mPosition = mTrajectory.front().position;
	mVelocity = mTrajectory.front().velocity;
	mCarDirection = mTrajectory.front().direction;
	mTrajectory.pop();
}

void CarLogic::insertInputs(sf::Time serverTime, Inputs inputs)
{
	mServerInputs.emplace(serverTime, inputs);
}

void CarLogic::setCarDirection(sf::Vector2f d)
{
	mCarDirection = d;
}

/*
ProjectileLogic* CarLogic::instanciateProjectile(int dmg, sf::Time lifetime, float speed, sf::Vector2f pos, sf::Vector2f direction, sf::RectangleShape rect)
{
	return new ProjectileLogic(dmg, lifetime, speed, pos, direction, rect, this);
}*/