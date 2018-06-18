#pragma once
#include <algorithm>

//�ṩһЩ��Ϸ���õ�����ѧ�㷨
class GameMath
{
public:
	static float FInterpTo(float Current, float Target, float DeltaTime, float InterSpeed)
	{
		if (InterSpeed <= 0.f)
		{
			return Target;
		}

		// ���յ�ľ���
		const float Dist = Target - Current;
	
		const float DeltaPercent = DeltaTime * InterSpeed;
		const float DeltaMove = (DeltaPercent > 0.f && DeltaPercent < 1.f) ? Dist * DeltaPercent : Dist * (DeltaPercent <= 0.f ? 0.f : 1.f);
		
		return Current + DeltaMove;
	}
};