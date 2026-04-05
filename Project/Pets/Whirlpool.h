#ifndef WHIRLPOOL_H_
#define WHIRLPOOL_H_

#include "../GameObjects/GameObject.h"

class Whirlpool : public GameObject
{
public:
	GO_TYPE GetGOType() const override { return GO_TYPE::WHIRLPOOL; };
	void Update(double dt) override;
	void OnCollide(CollisionData& other) override;
	Whirlpool& SetupWhirlpool(float _duration, float _strength);

private:
	float duration{}, timer{}, strength{};
	float deltaTime{};
};

#endif // WHIRLPOOL_H_

