#pragma once
#include <algorithm>

//提供一些游戏中用到的数学算法
class GameMath
{
public:
	static float FInterpTo(float Current, float Target, float DeltaTime, float InterSpeed)
	{
		if (InterSpeed <= 0.f)
		{
			return Target;
		}

		// 到终点的距离
		const float Dist = Target - Current;
	
		const float DeltaPercent = DeltaTime * InterSpeed;
		const float DeltaMove = (DeltaPercent > 0.f && DeltaPercent < 1.f) ? Dist * DeltaPercent : Dist * (DeltaPercent <= 0.f ? 0.f : 1.f);
		
		return Current + DeltaMove;
	}
};