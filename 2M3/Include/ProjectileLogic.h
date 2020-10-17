#pragma once
#include <Entity.h>

class CarLogic; // CarLogic forward declaration

class ProjectileLogic : public Entity
{
public:
	ProjectileLogic(int dmg, sf::Time lifetime, float speed, sf::Vector2f pos, sf::Vector2f direction, sf::RectangleShape rect, CarLogic* car);
	ProjectileLogic(int dmg, sf::Time lifetime, float speed, float detection, sf::Vector2f pos, sf::Vector2f direction, sf::RectangleShape rect, CarLogic* car);

	void update(sf::Time dt, std::vector<Entity*> entities, std::vector<Entity*>& newEntities, std::set<Pair>& pairs) override;
	void onCollision(Entity* other) override;

	int getDamage();
	CarLogic* getCar();
	bool isGuided();

protected:
	int mDamage;
	float mMaxSpeed;
	sf::Time mLifetime;

	bool mGuided;
	Entity* mTarget;
	float mDetectionRange;
	float mGuideRate;

	CarLogic* mCar;
};