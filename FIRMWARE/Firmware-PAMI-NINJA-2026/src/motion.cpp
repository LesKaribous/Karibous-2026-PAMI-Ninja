// motion.h
#include "motion.h"

// Position absolue du robot
Pose currentPose  = {0.0f, 0.0f, 0.0f};
Pose targetPose   = {0.0f, 0.0f, 0.0f};
// Déplacement cible polaire
PolarMove targetMove = {0.0f, 0.0f, 0.0f};
float tempTargetRotation = 0.0f;
bool newPolarTarget = false;

int motionState = MOTION_WAIT;

int stepMultiplier = 8;
float circumferenceMM = WHEEL_DIAMETER_MM * PI;

// Variable Opponent
bool opponentChecking = false;

// create the stepper motor object
AccelStepper motor_G(AccelStepper::DRIVER, STEP_G, DIR_G);
AccelStepper motor_D(AccelStepper::DRIVER, STEP_D, DIR_D);

static float normalizeAngleDegrees(float angle){
  while (angle > 180.0f) angle -= 360.0f;
  while (angle < -180.0f) angle += 360.0f;
  return angle;
}

static void computeApproachToTarget(float targetAngleDegrees, bool allowReverseApproach) {
  // Rotation initiale pour s'aligner vers la cible.
  targetMove.rotation1 = normalizeAngleDegrees(targetAngleDegrees - currentPose.rot);

  // Sans orientation finale imposee, on peut reculer pour limiter la rotation.
  if (allowReverseApproach && fabs(targetMove.rotation1) > 90.0f){
    targetMove.rotation1 = normalizeAngleDegrees(targetMove.rotation1 + 180.0f);
    targetMove.distance = -targetMove.distance;
  }

  tempTargetRotation = normalizeAngleDegrees(currentPose.rot + targetMove.rotation1);
}

void initMotion(){
  // Configure les pins
  pinMode(EN,OUTPUT);
  pinMode(DIR_G,OUTPUT);
  pinMode(STEP_G,OUTPUT);
  pinMode(DIR_D,OUTPUT);
  pinMode(STEP_D,OUTPUT);

  // Desactive les moteurs
  digitalWrite(EN,HIGH);
  // Configure les vitesses et accelerations
  setMaxSpeed();
  setAcceleration();
  // Motion est ready
  setMotionState(MOTION_RUN);
}

void setMotionState(bool _state){
  motionState = _state;
}

void setMaxSpeed(float _maxSpeed){
   motor_G.setMaxSpeed(_maxSpeed);       // steps/s
   motor_D.setMaxSpeed(_maxSpeed);       // steps/s
}

void setAcceleration(float _acceleration){
   motor_G.setAcceleration(_acceleration); // steps/s^2 
   motor_D.setAcceleration(_acceleration); // steps/s^2 
}

void enableMotors(){
  digitalWrite(EN,LOW);
}

void disableMotors(){
  digitalWrite(EN,HIGH);
}

void go(float _dist){
  if (_dist > 0.0f) setOpponentDetectionDirection(OPPONENT_DIR_FORWARD);
  else if (_dist < 0.0f) setOpponentDetectionDirection(OPPONENT_DIR_BACKWARD);
  else setOpponentDetectionDirection(OPPONENT_DIR_NONE);

  long stepValue = convertDistToStep(_dist);
  motor_G.move(stepValue);
  motor_D.move(-stepValue);
  processMove();
  setOpponentDetectionDirection(OPPONENT_DIR_NONE);
}

void turn(float _angle){
  long stepValue = convertAngleToStep(_angle);
  bool previousOpponentChecking = opponentChecking ;
  motor_G.move(stepValue);
  motor_D.move(stepValue);
  setOpponentChecking(false);
  processMove();
  setOpponentChecking(previousOpponentChecking);
}

void updateMotors(){
  updateMatchTime();
  if(getMatchState() != PAMI_STOP){
    enableMotors();
    motor_D.run();
    motor_G.run();
  }
  else if (getMatchState() == PAMI_STOP)
    disableMotors();
}

void setCurrentY(float _y){
  currentPose.setY(_y);
}

void setCurrentX(float _x){
  currentPose.setX(_x);
}

void setCurrentRot(float _rot){
  currentPose.setRot(_rot);
}

float getCurrentY(){
  return currentPose.getY();
}

float getCurrentX(){
  return currentPose.getX();
}

float getCurrentRot(){
  return currentPose.getRot();
}

void setOpponentChecking(bool _opponentChecking){
  opponentChecking = _opponentChecking;
  setOpponentMonitoring(_opponentChecking);
}

void setMotionState(int _motionState){
  motionState = _motionState;
}

void processMove(){

  long tempDistance_D = 0;
  long tempDistance_G = 0;

  debug("Processing Move...");
  while((motor_D.isRunning() || motor_G.isRunning())&& getMatchState() != PAMI_STOP){
    updateMotors();
    if (opponentChecking){
      //distanceToCheck = convertStepToDist(motor_D.distanceToGo()); // TODO : Changer pour utiliser l'acceleration ou la vitesse ? 
      //debug ("check at" + String(distanceToCheck) + "mm");

      if (isOpponentDetected()){
        //debug ("Opponent at" + String(distanceToGo) + "mm");

        tempDistance_D = motor_D.distanceToGo();
        tempDistance_G = motor_G.distanceToGo();

        setAcceleration(STOP_ACCELERATION);
        setMaxSpeed(STOP_SPEED);

        motor_D.move(0);
        motor_G.move(0);

        tempDistance_D = tempDistance_D + motor_D.distanceToGo();
        tempDistance_G = tempDistance_G + motor_G.distanceToGo();

        updateMotors();
        while(motor_D.isRunning() || motor_G.isRunning()) updateMotors();
        while(isOpponentDetected()&& getMatchState() != PAMI_STOP){
          updateMatchTime();
          //debug ("Opponent at" + String(distanceToGo) + "mm");
          //readSensors(true);
        }
        if(getMatchState() == PAMI_STOP) debug("Movement resumed");
        else {
          debug ("Freeway");
          setAcceleration(MAX_ACCELERATION);
          setMaxSpeed(MAX_SPEED);

          motor_D.move(tempDistance_D);
          motor_G.move(tempDistance_G);
        }
      }
    }
  }
  if(getMatchState() == PAMI_STOP) debug("Movement resumed");
  else debug("Movement ok");
}

long convertDistToStep(float _dist) {
  float revolutions = _dist / circumferenceMM;
  return static_cast<long>(revolutions * STEPS_PER_REVOLUTION * stepMultiplier);
}

long convertStepToDist(long _step) {
  float revolutions = abs(_step) / (STEPS_PER_REVOLUTION * stepMultiplier);
  return static_cast<long>(revolutions * circumferenceMM);
}

// Convert angle to step for one wheel (robot turning on center)
long convertAngleToStep(float _angle) {
    float angleRadians = _angle * (PI / 180.0f); // Conversion en radians
    float arcLength = (WHEEL_DISTANCE_MM / 2.0f) * angleRadians; // Longueur de l'arc parcouru par chaque roue
    float revolutions = arcLength / circumferenceMM;
    return static_cast<long>(revolutions * STEPS_PER_REVOLUTION * stepMultiplier);
}

void convertToPolar(Pose _target){
  convertToPolar(_target.x, _target.y, _target.rot);
}

void convertToPolar(float _x, float _y){
  float dx = _x - currentPose.x;
  float dy = _y - currentPose.y;

  float targetAngleRadians = atan2(-dy, dx);
  float targetAngleDegrees = targetAngleRadians * (180.0f / M_PI);

  targetMove.distance = sqrt(dx*dx + dy*dy);
  computeApproachToTarget(targetAngleDegrees, true);

  targetMove.rotation2 = 0; // Pas de rotation finale

  newPolarTarget = true;
}

void convertToPolar(float _x, float _y, float _rot){
  float dx = _x - currentPose.x;
  float dy = _y - currentPose.y;

  float targetAngleRadians = atan2(-dy, dx);
  float targetAngleDegrees = targetAngleRadians * (180.0f / M_PI);
  float targetRotDegrees = _rot;

  targetMove.distance = sqrt(dx*dx + dy*dy);
  computeApproachToTarget(targetAngleDegrees, false);

  // Calculer la rotation la plus courte pour rotation2
  targetMove.rotation2 = normalizeAngleDegrees(targetRotDegrees - tempTargetRotation);

  newPolarTarget = true;
}

void goRelative(float _dist){
  go(_dist);
  float rotRad = currentPose.rot * (PI / 180.0f);
  currentPose.setX(currentPose.x + _dist * cos(rotRad));
  currentPose.setY(currentPose.y - _dist * sin(rotRad));
}

void turnRelative(float _angle){
  turn(_angle);
  currentPose.setRot(normalizeAngleDegrees(currentPose.rot + _angle));
}

void goTo(Pose _target){
  goTo(_target.x, _target.y, _target.rot);
}

void goTo(float _x, float _y){
  convertToPolar(_x,_y);
  turn(targetMove.rotation1);
  go(targetMove.distance);
  currentPose.setX(_x);
  currentPose.setY(_y);
  currentPose.setRot(tempTargetRotation);
  newPolarTarget = false;
}

void goTo(float _x, float _y, float _rot){
  convertToPolar(_x,_y,_rot);
  turn(targetMove.rotation1);
  go(targetMove.distance);
  currentPose.setX(_x);
  currentPose.setY(_y);
  turn(targetMove.rotation2);
  currentPose.setRot(_rot);
  newPolarTarget = false;
}
