#ifndef _BONERIG_H
#define _BONERIG_H

#include <SDL.h>
#include <stdbool.h>

#include "../util.h"

#define BA_TRANSITION	  (-0xDE)
#define BA_TRANSITION_END (-0xAD)

typedef struct _polar_pos {
	/* Radians */
	float angle;
	float length;
} polar_pos;

#define KEYFRAME_DATA                                                          \
	/* Degrees */                                                              \
	double angle;

typedef struct _Bone Bone;
typedef struct _Bone {
	Bone	 *parent;
	polar_pos local;
	struct KeyframeData {
		KEYFRAME_DATA
	} anim_data;
} Bone;

typedef struct _SkinRig {
	Bone	  *bone;
	SDL_Rect   subimage;
	SDL_FPoint center;
} SkinRig;

#define MAX_KEYFRAMES 32

typedef struct _BoneKeyframeData {
	/* Bone to animate */
	Bone *bone;
	KEYFRAME_DATA
} BoneKeyframeData;

typedef struct _BoneKeyframe {
	/* The moment of this keyframe, in seconds */
	float moment;
	/* Keyframe data for each bone */
	BoneKeyframeData data[MAX_KEYFRAMES];
} BoneKeyframe;

typedef struct _BoneAnimation {
	/* Current animation time step */
	float time;
	/* Number of keyframes */
	size_t keyframe_count;
	/* Number of bones on each keyframe */
	size_t keyframe_bone_count;
	/* Keyframes. Bones have to remain in the same order throughout the
	 * animation, do not set bones in a different order at different
	 *keyframes. */
	BoneKeyframe keyframes[MAX_KEYFRAMES];
} BoneAnimation;

/**
 * Calculates the world position of a bone based on its local position and angle
 * relative to its parent
 * @param bone The bone to calculate the world position of
 * @param theta The angle (radians) to rotate the whole bone
 * @param x Ptr to output the x position of the bone in the world
 * @param y Ptr to output the y position of the bone in the world
 * @param alpha Ptr to output the angle (radians) of the bone in the world
 */
void bone_get_world_position(Bone *bone, float theta, float *x, float *y,
							 float *alpha, bool fliph);

void step_animation(BoneAnimation *animation, float dt);

void play_animation(BoneAnimation	   **dst_animation,
					const BoneAnimation *animation_to_play, bool force);

#endif // _BONERIG_H
