#pragma once
#include <SFML/Graphics/Rect.hpp>
#include "CBoxCollider.h"
#include "CSprite.h"

inline sf::FloatRect ComputeWorldAttackAABB(const CBoxCollider* collider, const CSprite* sprite)
{
    if (!collider || !sprite) return sf::FloatRect();

    const sf::FloatRect bodyAABB = collider->GetAABB();
    const sf::FloatRect attackRect = collider->GetAttackAABB();
    if (attackRect.size.x <= 0.0f || attackRect.size.y <= 0.0f) return sf::FloatRect();

    const sf::Vector2f offset = collider->GetAttackAABBOffset();

    float attackX = 0.0f;
    if (sprite->GetDirection() == Direction::Right)
        attackX = (bodyAABB.position.x + bodyAABB.size.x) + offset.x;
    else
        attackX = bodyAABB.position.x - offset.x - attackRect.size.x;

    const float attackY = bodyAABB.position.y + offset.y;
    return sf::FloatRect({ attackX, attackY }, { attackRect.size.x, attackRect.size.y });
}