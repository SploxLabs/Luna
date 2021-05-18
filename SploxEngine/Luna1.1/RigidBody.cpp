#include "RigidBody.h"

XMVECTOR RigidBody::GetOrientation() {
	XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(orientation.x, orientation.y, orientation.z);
	return XMVector3Transform(forward, rotation);
}

void RigidBody::Update(float DT) {
	/* translational forces and updates */
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR vel = XMLoadFloat3(&velocity);

	XMVECTOR force = XMVectorZero();
	for (int i = 0, end = forces.size(); i < end; ++i) {
		force = XMVectorAdd(force, forces.at(i).force);
	}

	if (thurster_activation && fuel > 0) {
		XMVECTOR thruster = XMVector3Transform(XMVectorSet(0,1,0,0) * thruster_force, GetOrientationMatrix() );
		force = XMVectorAdd(force, thruster);
		fuel -= fuel_rate * DT;
	}

	//forward eular approximation
	vel = XMVectorAdd(vel, force / mass * DT);
	pos = XMVectorAdd(pos, vel * DT);

	XMStoreFloat3(&position, pos);
	XMStoreFloat3(&velocity, vel);

	//angle shit	
	
	XMVECTOR ang_vel = XMLoadFloat3(&angular_velocity);
	XMVECTOR orientation_angles = XMLoadFloat3(&orientation);

	float moment_of_inertia = moment_of_inertia_coefficient * mass * radius * radius;
	
	XMFLOAT3 ang_acc = { 0, 0, 0 };
	if (roll_left && !roll_right) {
		ang_acc.x = angluar_acceleration_strength / moment_of_inertia;
	}
	if (roll_right && !roll_left) {
		ang_acc.x = -angluar_acceleration_strength / moment_of_inertia;
	}
	if (pitch_up && !pitch_down) {
		ang_acc.y = angluar_acceleration_strength / moment_of_inertia;
	}
	if (pitch_down && !pitch_up) {
		ang_acc.y = -angluar_acceleration_strength / moment_of_inertia;
	}
	if (yaw_left && !yaw_right) {
		ang_acc.z = angluar_acceleration_strength / moment_of_inertia;
	}
	if (yaw_right && !yaw_left) {
		ang_acc.z = -angluar_acceleration_strength / moment_of_inertia;
	}

	XMVECTOR ang_acc_vec = XMLoadFloat3(&ang_acc);

	//ang_acc_vec = XMVector3Transform(ang_acc_vec, GetOrientationMatrix());

	ang_vel = XMVectorAdd(ang_vel, ang_acc_vec* DT);
	orientation_angles = XMVectorAdd(orientation_angles, ang_vel * DT);

	XMStoreFloat3(&angular_velocity, ang_vel);
	XMStoreFloat3(&orientation, orientation_angles);
}
