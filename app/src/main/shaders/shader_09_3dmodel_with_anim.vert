#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec2 inUV1; // We don't use it
layout (location = 4) in vec4 inJoint0;
layout (location = 5) in vec4 inWeight0;

layout (set = 0, binding = 0) uniform UBOScene
{
	mat4 projection;
	mat4 view;
	vec4 lightPos;
} uboScene;

#define MAX_NUM_JOINTS 128

layout (set = 2, binding = 0) uniform UBONode {
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	float jointCount;
} node;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main()
{
	vec4 pos;
	if (node.jointCount > 0.0) {
		// Mesh is skinned
		mat4 skinMat =
		inWeight0.x * node.jointMatrix[int(inJoint0.x)] +
		inWeight0.y * node.jointMatrix[int(inJoint0.y)] +
		inWeight0.z * node.jointMatrix[int(inJoint0.z)] +
		inWeight0.w * node.jointMatrix[int(inJoint0.w)];

		pos = uboScene.projection * uboScene.view * node.matrix * skinMat * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(uboScene.view * node.matrix * skinMat))) * inNormal);
	} else {
		pos = uboScene.projection * uboScene.view * node.matrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(uboScene.view * node.matrix))) * inNormal);
	}

	pos.y = -pos.y;
	gl_Position = pos;

	outColor = vec3(1.0, 1.0, 1.0);
	outUV = inUV;

	vec3 lPos = mat3(uboScene.view) * uboScene.lightPos.xyz;
	outLightVec = lPos - pos.xyz;
	outViewVec = -pos.xyz;
}