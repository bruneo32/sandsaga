#include "bonerig.h"

void bone_get_world_position(Bone *bone, float theta, float *x, float *y,
							 float *alpha, bool fliph) {
	float parent_x = 0.0f, parent_y = 0.0f, parent_angle = 0.0f;
	float world_angle = bone->local.angle;

	/* Horizontal mirror the bone origin when fliph is true */
	if (fliph) {
		const float wan = fmodf(world_angle, M_2PI);
		world_angle		= fmodf((M_PI_2 - wan), M_2PI);
	}

	if (bone->parent != NULL) {
		/* Get the parent's propagated global position */
		bone_get_world_position(bone->parent, theta, &parent_x, &parent_y,
								&parent_angle, fliph);
		/* Apply the parent's global rotation */
		world_angle = parent_angle + world_angle;
	} else {
		/* Apply the bone's local rotation. This propagates from the root bone
		 * to all of the childs. */
		world_angle += theta;
	}

	/* Calculate the world position of the current bone */
	float world_x = parent_x + (bone->local.length * cosf(world_angle));
	float world_y = parent_y + (bone->local.length * sinf(world_angle));

	if (x)
		*x = world_x;
	if (y)
		*y = world_y;
	if (alpha)
		*alpha = world_angle;
}

void step_animation(BoneAnimation *animation, float dt) {
	if (animation->keyframe_count == 0)
		return;

	BoneKeyframe *first_keyframe = &animation->keyframes[0];
	BoneKeyframe *last_keyframe =
		&animation->keyframes[animation->keyframe_count - 1];

	const float duration = last_keyframe->moment;

	BoneKeyframe *merger_keyframe =
		(first_keyframe->moment == BA_TRANSITION) ? first_keyframe : NULL;

	BoneKeyframe *current_keyframe = NULL;
	BoneKeyframe *next_keyframe	   = NULL;

	/* Invalidate merger keyframe when a round has passed */
	animation->time += dt;
	if (animation->time >= duration && merger_keyframe != NULL) {
		merger_keyframe->moment = BA_TRANSITION_END;
		merger_keyframe			= NULL;
	}
	/* Clip animation to duration */
	animation->time = fmodf(animation->time, duration);

	float lerp_value = 0.0f;

	for (size_t i = 0; i < animation->keyframe_count; ++i) {
		BoneKeyframe *keyframe = &animation->keyframes[i];

		/* Skip merger */
		if (i == 0 && (keyframe->moment == BA_TRANSITION_END ||
					   keyframe->moment == BA_TRANSITION))
			continue;

		BoneKeyframe *keyframe_prev = (keyframe == first_keyframe)
										  ? last_keyframe
										  : &animation->keyframes[i - 1];

		float start_time = keyframe_prev->moment;
		float end_time	 = keyframe->moment;

		/* Adjust time */
		if (start_time == BA_TRANSITION || start_time == BA_TRANSITION_END) {
			start_time = 0.0f;
			if (keyframe_prev->moment == BA_TRANSITION_END)
				keyframe_prev = last_keyframe;
		}

		if (animation->time >= start_time && animation->time < end_time) {
			current_keyframe = keyframe_prev;
			next_keyframe	 = keyframe;
			lerp_value =
				(animation->time - start_time) / (end_time - start_time);

			break;
		}
	}

	if (current_keyframe != NULL) {
		for (size_t i = 0; i < animation->keyframe_bone_count; ++i) {
			BoneKeyframeData *current_keyframe_data =
				&current_keyframe->data[i];
			BoneKeyframeData *next_keyframe_data = &next_keyframe->data[i];

			Bone *bone = current_keyframe_data->bone;
			if (!bone)
				continue;

			bone->anim_data.angle = lerp(current_keyframe_data->angle,
										 next_keyframe_data->angle, lerp_value);
		}
	}
}

void play_animation(BoneAnimation	   **dst_animation,
					const BoneAnimation *animation_to_play, bool force) {
	/* Do nothing if the animation hasn't changed */
	if (!(force || *dst_animation != animation_to_play))
		return;

	*dst_animation		   = (BoneAnimation *)animation_to_play;
	(*dst_animation)->time = 0.0f;

	/* Use the first frame as a merger */
	/* Merger use the first keyframe if it's BA_TRANSITION */
	if ((*dst_animation)->keyframe_count > 0) {
		BoneKeyframe *ref_keyframe = &(*dst_animation)->keyframes[0];

		if (ref_keyframe->moment == BA_TRANSITION_END)
			/* Enable merger when previously disabled */
			ref_keyframe->moment = BA_TRANSITION;
		else if (ref_keyframe->moment != BA_TRANSITION)
			/* Not a merger */
			return;

		size_t ref_keyframe_count = (*dst_animation)->keyframe_bone_count;

		for (size_t i = 0; i < ref_keyframe_count; ++i) {
			BoneKeyframeData *ref_keyframe_data = &ref_keyframe->data[i];

			Bone *bone = ref_keyframe_data->bone;
			if (!bone)
				continue;

			ref_keyframe_data->angle = bone->anim_data.angle;
		}
	}
}
