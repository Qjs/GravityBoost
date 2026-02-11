#include <box2d/box2d.h>
#include <math.h>

static inline float clampf(float x, float a, float b) {
  return x < a ? a : (x > b ? b : x);
}

static b2Vec2 v2_add(b2Vec2 a, b2Vec2 b){ return (b2Vec2){a.x+b.x, a.y+b.y}; }
static b2Vec2 v2_sub(b2Vec2 a, b2Vec2 b){ return (b2Vec2){a.x-b.x, a.y-b.y}; }
static b2Vec2 v2_mul(b2Vec2 a, float s){ return (b2Vec2){a.x*s, a.y*s}; }

static float v2_len(b2Vec2 v){ return sqrtf(v.x*v.x + v.y*v.y); }

static b2Vec2 v2_norm_or_zero(b2Vec2 v){
  float l = v2_len(v);
  if (l > 1e-6f) return v2_mul(v, 1.0f/l);
  return (b2Vec2){0,0};
}

// Apply repulsion to keep bodies from clumping.
// strength is "acceleration-ish" in m/s^2 (we multiply by mass to get force).
void Fleet_ApplySeparation(
    b2BodyId* bodies, int count,
    float r_sep,
    float strength,
    float max_accel
){
  float r2 = r_sep * r_sep;

  for (int i = 0; i < count; i++) {
    b2Vec2 pi = b2Body_GetPosition(bodies[i]);

    b2Vec2 accel = {0,0};

    for (int j = 0; j < count; j++) {
      if (j == i) continue;

      b2Vec2 pj = b2Body_GetPosition(bodies[j]);
      b2Vec2 d  = v2_sub(pi, pj); // away from j
      float d2  = d.x*d.x + d.y*d.y;

      if (d2 >= r2) continue;

      // closer => stronger push (smooth)
      float dist = sqrtf(d2 + 1e-6f);
      float t = 1.0f - (dist / r_sep);   // 0..1
      float w = t * t;                   // ease-in

      b2Vec2 dir = v2_mul(d, 1.0f / (dist + 1e-6f));
      accel = v2_add(accel, v2_mul(dir, strength * w));
    }

    // Clamp acceleration
    float a_len = v2_len(accel);
    if (a_len > max_accel) accel = v2_mul(accel, max_accel / (a_len + 1e-6f));

    // Convert accel -> force and apply at COM
    float m = b2Body_GetMass(bodies[i]);
    b2Vec2 F = v2_mul(accel, m);
    b2Body_ApplyForceToCenter(bodies[i], F, true);
  }
}


typedef enum FleetMode { FLEET_STAR, FLEET_V, FLEET_CHAIN, FLEET_RING } FleetMode;

typedef struct Fleet {
  int count;
  b2BodyId bodies[10];

  // base tethers (always present)
  b2JointId base_to_leader[10]; // [1..count-1]

  // overlay joints (vary by mode)
  b2JointId overlay[32];
  int overlay_count;

  FleetMode mode;
} Fleet;


#include <box2d/box2d.h>

static b2JointId CreateSpring(
    b2WorldId world,
    b2BodyId a, b2BodyId b,
    b2Vec2 worldAnchorA,
    b2Vec2 worldAnchorB,
    float restLength,
    float hertz,
    float dampingRatio,
    bool collideConnected
){
    b2DistanceJointDef jd = b2DefaultDistanceJointDef();
    jd.bodyIdA = a;
    jd.bodyIdB = b;
    jd.collideConnected = collideConnected;

    jd.localAnchorA = b2Body_GetLocalPoint(a, worldAnchorA);
    jd.localAnchorB = b2Body_GetLocalPoint(b, worldAnchorB);

    jd.length = restLength;

    jd.enableSpring = true;
    jd.hertz = hertz;
    jd.dampingRatio = dampingRatio;

    // Optional: limits (if your build exposes them)
    // jd.enableLimit = true;
    // jd.minLength = restLength * 0.85f;
    // jd.maxLength = restLength * 1.25f;

    return b2CreateDistanceJoint(world, &jd);
}

void Fleet_CreateStarTethers(
    b2WorldId world,
    b2BodyId* bodies,
    int count,
    float restLength,
    float hertz,
    float dampingRatio
){
    b2BodyId leader = bodies[0];
    b2Vec2 leaderPos = b2Body_GetPosition(leader);

    for (int i = 1; i < count; i++) {
        b2Vec2 p = b2Body_GetPosition(bodies[i]);

        CreateSpring(
            world,
            leader, bodies[i],
            leaderPos, p,                 // anchors at current positions
            restLength,
            hertz,
            dampingRatio,
            false
        );
    }
}


void Fleet_CreateTwoPointTethers(
    b2WorldId world,
    b2BodyId* bodies,
    int count,
    float restLength,
    float hertz,
    float dampingRatio
){
    b2BodyId leader = bodies[0];

    // Two anchor points on leader in LOCAL coordinates (meters)
    // (tune based on ship shape/size)
    b2Vec2 aL_local = (b2Vec2){ -0.35f, 0.0f };
    b2Vec2 aR_local = (b2Vec2){  0.35f, 0.0f };

    // Convert to world points each time we create joints
    b2Vec2 aL_world = b2Body_GetWorldPoint(leader, aL_local);
    b2Vec2 aR_world = b2Body_GetWorldPoint(leader, aR_local);

    for (int i = 1; i < count; i++) {
        b2BodyId f = bodies[i];
        b2Vec2 fPos = b2Body_GetPosition(f);

        // two springs to reduce “spinning around the leader”
        CreateSpring(world, leader, f, aL_world, fPos, restLength, hertz, dampingRatio, false);
        CreateSpring(world, leader, f, aR_world, fPos, restLength, hertz, dampingRatio, false);
    }


    void Fleet_AddNeighborSprings(
    b2WorldId world,
    b2BodyId* bodies,
    int count,
    float neighborRest,
    float neighborHertz,
    float neighborDamping
){
    // followers are [1..count-1]
    for (int i = 1; i < count; i++) {
        int j = (i == count - 1) ? 1 : (i + 1);

        b2Vec2 pi = b2Body_GetPosition(bodies[i]);
        b2Vec2 pj = b2Body_GetPosition(bodies[j]);

        CreateSpring(world, bodies[i], bodies[j], pi, pj, neighborRest,
                     neighborHertz, neighborDamping, false);
    }
}

b2Vec2 a = PlanetGravityAccel(pos);
float m = b2Body_GetMass(body);
b2Body_ApplyForce(body, (b2Vec2){ m*a.x, m*a.y }, pos, true);