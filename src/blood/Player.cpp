#include "Player.h"
#include "HitBox.h"
#include "GameScreen.h"
#include "BongoScreen.h"
#include "Fade.h"

#include <cmath>

void Player::onInit()
{
  speed = 4;
  model = getResources()->load<Model>("models/curuthers/curuthers");
  pitchfork = getResources()->load<Texture>("sprites/pitchfork");
  heart = getResources()->load<Texture>("sprites/heart");
  health = 100;

  getEntity()->addComponent<Camera>();
  //getEntity()->addComponent<Fade>();
}

void Player::doAttack()
{
  //std::cout << "Attack" << std::endl;
  std::shared_ptr<Entity> hbe = getWorld()->addEntity();
  hbe->getTransform()->setRotation(getTransform()->getRotation());
  hbe->getTransform()->setPosition(getTransform()->getPosition());
  hbe->getTransform()->translate(Vector3(0, 0, 1));
  hbe->addComponent<HitBox>(getEntity());
}

void Player::checkHits()
{
  std::vector<std::shared_ptr<Entity> > entities;

  getWorld()->getEntities<HitBox>(entities);

  for(std::vector<std::shared_ptr<Entity> >::iterator it = entities.begin();
    it != entities.end(); it++)
  {
    std::shared_ptr<HitBox> hb = (*it)->getComponent<HitBox>();

    if(hb->getOwner() == getEntity()) continue;

    Vector3 diff = getTransform()->getPosition() - (*it)->getTransform()->getPosition();
    float len = fabs(diff.x) + fabs(diff.y) + fabs(diff.z);

    if(len > 2) continue;

    health -= 25;

    if(health < 0) health = 0;

    if(health <= 0)
    {
      getWorld()->reset();
      getWorld()->addEntity<GameScreen>();
    }

    diff = diff * -1;
    diff.y = 0.5f;
    getTransform()->translate(diff);
    return;
  }
}

void Player::onTick()
{
  if(getKeyboard()->isKeyPressed(KEY_T))
  {
    getWorld()->reset();
    getWorld()->addEntity<GameScreen>();
  }
  else if(getKeyboard()->isKeyPressed(KEY_Y))
  {
    getWorld()->reset();
    getWorld()->addEntity<BongoScreen>();
  }

  if(getKeyboard()->isKeyDown(KEY_LEFT))
  {
    getTransform()->setRotation(
      getTransform()->getRotation() + Vector3(0, 90, 0) * getEnvironment()->getDeltaTime());
  }
  else if(getKeyboard()->isKeyDown(KEY_RIGHT))
  {
    getTransform()->setRotation(
      getTransform()->getRotation() + Vector3(0, -90, 0) * getEnvironment()->getDeltaTime());
  }

  Vector3 lp = getTransform()->getPosition() + Vector3(0, -1, 0);

  getTransform()->translate(Vector3(0, -4, 0) * getEnvironment()->getDeltaTime());

  if(getKeyboard()->isKeyDown(KEY_UP))
  {
    getTransform()->translate(Vector3(0, 0, speed) * getEnvironment()->getDeltaTime());
  }
  else if(getKeyboard()->isKeyDown(KEY_DOWN))
  {
    getTransform()->translate(Vector3(0, 0, -speed) * getEnvironment()->getDeltaTime());
  }

  Vector3 np = getTransform()->getPosition() + Vector3(0, -1, 0);

  bool solved = false;
  Vector3 sp = smc->getCollisionResponse(np, Vector3(1, 2, 1), solved, lp);

  if(solved)
  {
    np = sp;
  }
  else
  {
    np = lp;
  }

  np = np + Vector3(0, 1, 0);
  getTransform()->setPosition(np);

  if(getKeyboard()->isKeyDown(KEY_SPACE))
  {
    if(attack == 0)
    {
      attack = 0.01f;
    }
  }

  if(attack > 0)
  {
    float la = attack;
    attack += 6 * getEnvironment()->getDeltaTime();

    if(la < 2 && attack >= 2)
    {
      doAttack();
    }
  }

  if(attack >= 3) attack = 0;

  checkHits();
}

void Player::onGui()
{
  Vector4 clip = Vector4(0, 0, pitchfork->getWidth() / 3, pitchfork->getHeight());

  clip.x = clip.z * (int)attack;
  //std::cout << clip.x << std::endl;

  Vector2 pos(
    getWindow()->getBuffer()->getWidth() / 2.0f - clip.z / 2.0f,
    getWindow()->getBuffer()->getHeight() - clip.w);

  getGui()->image(pos, pitchfork, clip);
  clip = Vector4(0, 0, heart->getWidth(), heart->getHeight());

  for(int i = 0; i < 4; i++)
  {
    if(health <= i * 25) break;

    pos = Vector2(
      10 + i * clip.z,
      getWindow()->getBuffer()->getHeight() - 10 - clip.w);

    getGui()->image(pos, heart);
  }
}
