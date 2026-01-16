#include "MatrixUtils.h"

AEMtx33 GetTransformMtx(AEVec2 translation, f32 rotationDeg, AEVec2 scaling)
{
    AEMtx33 scale = { 0 };
    AEMtx33Scale(&scale, scaling.x, scaling.y);
    AEMtx33 rotate = { 0 };
    AEMtx33Rot(&rotate, rotationDeg * PI/180); //Convert to rad
    AEMtx33 translate = { 0 };
    AEMtx33Trans(&translate, translation.x, translation.y);
    AEMtx33 transform = { 0 };
    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    return transform;
}

void GetTransformMtx(AEMtx33& mtx, AEVec2 translation, f32 rotationDeg, AEVec2 scaling) {
    AEMtx33 scale = { 0 };
    AEMtx33Scale(&scale, scaling.x, scaling.y);
    AEMtx33 rotate = { 0 };
    AEMtx33Rot(&rotate, rotationDeg * PI / 180); //Convert to rad
    AEMtx33 translate = { 0 };
    AEMtx33Trans(&translate, translation.x, translation.y);
    AEMtx33Concat(&mtx, &rotate, &scale);
    AEMtx33Concat(&mtx, &translate, &mtx);
}
