#ifndef TIME_H
#define TIME_H

float4 TimeInfo;

float GetDeltaTime() { return TimeInfo.x; }
float GetInvDeltaTime() { return TimeInfo.y; }
float GetElapsedTime() { return TimeInfo.z; }

#endif // TIME_H