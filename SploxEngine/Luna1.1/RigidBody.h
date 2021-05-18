#pragma once
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;

class RigidBody {
public:
	XMFLOAT3 position; //m
	XMFLOAT3 velocity; //m/s
	XMFLOAT3 angular_velocity;
	XMFLOAT3 orientation;

	float moment_of_inertia_coefficient;
	float radius;
	float mass; //kg
	float angluar_acceleration_strength;

	bool pitch_up;
	bool pitch_down;
	bool roll_left;
	bool roll_right;
	bool yaw_left;
	bool yaw_right;

	bool thurster_activation;
	float thruster_force;
	float fuel;
	float fuel_rate;
	float fuel_max;

	struct Force {
		Force(XMVECTOR FORCE) { force = FORCE; }
		XMVECTOR force;
	};

	struct Torque {
		Torque(XMVECTOR AXIS, float FORCE) { 
			axis = AXIS;
			force = FORCE;
		}
		XMVECTOR axis;
		float force;
	};

	XMVECTOR GetOrientation();
	XMMATRIX GetOrientationMatrix() { return XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&orientation)); }

	std::vector<Force> forces;
	std::vector<Torque> torques;

	void Update(float DT);//DT in seconds
	void AddConstantForce(XMVECTOR FORCE) { forces.push_back(Force(FORCE)); }
	void AddConstantTorque(XMVECTOR AXIS, float FORCE) { torques.push_back(Torque(AXIS, FORCE)); }
};

