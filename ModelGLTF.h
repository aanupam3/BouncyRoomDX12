#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ModelGLTF
{
	enum ComponentType {

		Invalid = 0,
		Byte = 5120,
		UByte = 5121,
		Short = 5122,
		UShort = 5123,
		UInt = 5125,
		Float = 5126
	};

	enum Target {
		ARRAY_BUFFER = 34962, // generally used for vertex buffer 
		ELEMENT_ARRAY_BUFFER = 34963 // generally use for index buffer
	};


	enum class AlphaMode {
		Opaque = 0, // The alpha value is ignored, and the rendered output is fully opaque.
		Mask, // The rendered output is either fully opaque or fully transparent depending on the alpha value and the specified alphaCutoff value
		Blend // The alpha value is used to composite the source and destination areas. The rendered output is combined with the background using the normal painting operation (i.e. the Porter and Duff over operator)
	};

	enum MeshPrimitiveMode { // The topology type of primitives to render.
		POINTS = 0,
		LINES = 1,
		LINE_LOOP = 2,
		LINE_STRIP = 3,
		TRIANGLES = 4,
		TRIANGLE_STRIP = 5,
		TRIANGLE_FAN = 6
	};

	struct Accessor {
		int bufferView{}; //The index of the bufferView
		int byteOffset{}; // The offset relative to the start of the buffer view in bytes.
		bool normalized{}; // Specifies whether integer data values are normalized before usage.
		ComponentType componentType{ ComponentType::Invalid }; // The datatype of the accessor’s components.
		std::string type{ "" }; // Specifies if the accessor’s elements are scalars, vectors ("VEC3", "VEC2" etc), or matrices.
		int count{ 0 }; // The number of elements referenced by this accessor.
		std::vector<float> max; // Maximum value of each component in this accessor
		std::vector<float> min; // Minimum value of each component in this accessor
		std::string name = "";

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Accessor, bufferView, byteOffset, normalized, componentType, type, count, max, min, name);
	};

	struct Buffer
	{
		int byteLength{ 0 }; // The length of the buffer in bytes.
		std::string uri{ "" }; // The URI (or IRI) of the buffer, i.e., name of the .bin to find it in
		std::string name{ "" };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Buffer, byteLength, uri, name);
	};

	struct BufferView
	{
		int buffer{}; // Index of the buffer in the list of buffers
		int byteLength{}; // The length of the bufferView in bytes.
		int byteOffset{}; // The offset into the buffer in bytes.
		int byteStride{ 1 }; // The stride in bytes between each vertex (default is 1 byte)
		std::string name{ "" };
		Target target{}; // The hint representing the intended GPU buffer type to use with this buffer view.

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(BufferView, byteLength, byteOffset, byteStride, target, name);
	};

	struct Image
	{
		std::string uri{ "" }; // The URI (or IRI) of the image.
		std::string mimeType{ "" }; // The image’s media type. This field MUST be defined when bufferView is defined
		int bufferView{}; // The index of the bufferView that contains the image. This field MUST NOT be defined when uri is defined.
		std::string name{ "" };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Image, uri, mimeType, bufferView, name);

	};


	struct Material
	{
		struct TextureInfo
		{
			int index{ -1 }; // The index of the texture in the list of textures
			int texCoord{}; // e.g. a value of 0 corresponds to TEXCOORD_0. A mesh primitive MUST have the corresponding texture coordinate attributes for the material to be applicable to it..

			NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TextureInfo, index, texCoord);
		};

		struct NormalTexture {
			int index{ -1 }; // The index of the texture in the list of textures
			int texCoord{}; // e.g. a value of 0 corresponds to TEXCOORD_0. A mesh primitive MUST have the corresponding texture coordinate attributes for the material to be applicable to it..
			float scale{ 1.0f }; // The scalar parameter applied to each normal vector of the normal texture. Scales the normal vector in X and Y directions using the formula: scaledNormal = normalize<sampled normal texture value> * 2.0 - 1.0) * vec3(<normal scale>, <normal scale>, 1.0.

			NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(NormalTexture, index, texCoord, scale);
		} normalTexture{};

		struct OcclusionTexture {
			int index{ -1 }; // The index of the texture in the list of textures
			int texCoord{}; // e.g. a value of 0 corresponds to TEXCOORD_0. A mesh primitive MUST have the corresponding texture coordinate attributes for the material to be applicable to it..
			float strength{ 1.0f }; // A scalar parameter controlling the amount of occlusion applied. A value of 0.0 means no occlusion. A value of 1.0 means full occlusion. This value affects the final occlusion value as: 1.0 + strength * (<sampled occlusion texture value> - 1.0)

			NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(OcclusionTexture, index, texCoord, strength);
		} occlusionTexture{}; // Occlusion is basically a shading factor that darkens areas that are more “enclosed.”

		struct PBRMetallicRoughness {
			std::vector<float> baseColorFactor{ 1.0f, 1.0f ,1.0f ,1.0f };// Defines linear multipliers for the sampled texels of the base color texture [R,G,B,A].
			TextureInfo baseColorTexture{};
			float metallicFactor{}; // This value defines a linear multiplier for the sampled metalness values of the metallic - roughness texture.
			float roughnessFactor{}; // This value defines a linear multiplier for the sampled roughness values of the metallic-roughness texture.
			TextureInfo metallicRoughnessTexture{};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
				PBRMetallicRoughness, baseColorFactor, baseColorTexture, metallicFactor, roughnessFactor, metallicRoughnessTexture);
		} pbrMetallicRoughness{};

		TextureInfo emissiveTexture{}; // controls the color and intensity of the light being emitted by the material
		std::vector<float> emissiveFactor{ 1.0f, 1.0f, 1.0f }; // This value defines linear multipliers for the sampled texels of the emissive texture.
		std::string name{ "" };
		std::string alphaMode{ "OPAQUE" };
		//AlphaMode alphaModeEnum{ AlphaMode::Opaque };
		float alphaCutoff{ 0.5f }; // Specifies the cutoff threshold when in MASK alpha mode. If the alpha value is greater than or equal to this value then it is rendered as fully opaque, otherwise, it is rendered as fully transparent. A value greater than 1.0 will render the entire material as fully transparent
		bool doubleSided{}; //  When this value is false, back-face culling is enabled. When this value is true, back-face culling is disabled and double-sided lighting is enabled. The back-face MUST have its normals reversed before the lighting equation is evaluated.

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Material, normalTexture, occlusionTexture, pbrMetallicRoughness, emissiveTexture,
			emissiveFactor, name, alphaMode, alphaCutoff, doubleSided);
	};

	struct MeshPrimitive
	{
		nlohmann::json attributes{}; // e.g., {"NORMAL": 1, "POSITION": 0, "TANGENT" : 2, "TEXCOORD_0"} : 3, a plain JSON object, where each key corresponds to a mesh attribute semantic and each value is the index of the accessor containing attribute’s data.
		int indices{}; // The index of the accessor that contains the vertex indices. When this is undefined, the primitive defines non-indexed geometry. When defined, the accessor MUST have SCALAR type and an unsigned integer component type.
		int material{}; // The index of the material in the materials list to apply to this primitive when rendering. 
		MeshPrimitiveMode mode{ MeshPrimitiveMode::TRIANGLES }; // The topology type of primitives to render.
		std::string targets{}; // An array of morph targets.

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
			MeshPrimitive, attributes, indices, material, mode, targets);
	};

	struct Mesh {
		std::string name{ "" };
		std::vector<MeshPrimitive> primitives{};
		std::vector<float> weights{}; // Array of weights to be applied to the morph targets. The number of array elements MUST match the number of morph targets.

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Mesh, name, primitives, weights);
	};

	// A node in the node hierarchy. 
	// When the node contains skin, all mesh.primitives MUST contain JOINTS_0 and WEIGHTS_0 attributes. 
	// A node MAY have either a matrix or any combination of translation/rotation/scale (TRS) properties. 
	// TRS properties are converted to matrices and postmultiplied in the T * R * S order to compose 
	// the transformation matrix: first the scale is applied to the vertices, then the rotation, and then the translation. 
	// If none are provided, the transform is the identity. 
	// When a node is targeted for animation (referenced by an animation.channel.target), 
	// matrix MUST NOT be present.
	struct Node {
		int camera{}; // The index of the camera referenced by this node. (not currently implemented)
		std::vector<int> children{}; // The indices of this node’s children in the scene hierarchy
		int skin{}; // The index of the skin referenced by this node. When a skin is referenced by a node within a scene, all joints used by the skin MUST belong to the same scene. When defined, mesh MUST also be defined
		std::vector<float> matrix{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }; // A floating-point 4x4 transformation matrix stored in column-major order.
		int mesh{ -1 }; // The index of the mesh (in the meshes list) in this node. -1 means no mesh
		std::vector<float> rotation{ 0,0,0,1 }; // The node’s unit quaternion rotation in the order (x, y, z, w), where w is the scalar.
		std::vector<float> scale{ 1,1,1 }; // The node’s non-uniform scale, given as the scaling factors along the x, y, and z axes.
		std::vector<float> translation{ 0,0,0 }; // The node’s translation along the x, y, and z axes.
		std::vector<float> weights{}; // The weights of the instantiated morph target. The number of array elements MUST match the number of morph targets of the referenced mesh. When defined, mesh MUST also be defined.
		std::string name{ "" };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Node, camera, children, skin, matrix, mesh, rotation, scale, translation, weights, name);
	};

	struct Sampler {
		enum class MagnificationFilter {
			NEAREST = 9728,
			LINEAR = 9729
		} magFilter{ MagnificationFilter::LINEAR };

		enum class MinificationFilter {
			NEAREST = 9728,
			LINEAR = 9729,
			NEAREST_MIPMAP_NEAREST = 9984,
			LINEAR_MIPMAP_NEAREST = 9985,
			NEAREST_MIPMAP_LINEAR = 9986,
			LINEAR_MIPMAP_LINEAR = 9987
		} minFilter{ MinificationFilter::LINEAR };

		enum WrappingMode {
			CLAMP_TO_EDGE = 33071,
			MIRRORED_REPEAT = 33648,
			REPEAT = 10497
		};
		WrappingMode wrapS{ WrappingMode::REPEAT }; // wrapping in the U direction
		WrappingMode wrapT{ WrappingMode::REPEAT }; // wrapping in the V direction

		std::string name{ "" };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
			Sampler, magFilter, minFilter, wrapS, wrapT, name);
	};

	struct Scene {
		std::vector<int> nodes; // The indices of each root node.
		std::string name{ "" };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Scene, nodes, name);
	};

	struct Texture {
		int sampler{}; // The index of the sampler (in the samplers list) used by this texture. When undefined, a sampler with repeat wrapping and auto filtering SHOULD be used
		int source{}; // The index of the image (in the images list) used by this texture. When undefined, an extension or other mechanism SHOULD supply an alternate texture source, otherwise behavior is undefined
		std::string name{ "" };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Texture, source, name);
	};

	struct ModelJson
	{
		std::vector<Accessor> accessors{};
		std::vector<Buffer> buffers{};
		std::vector<BufferView> bufferViews{};
		std::vector<Image> images{};
		std::vector<Material> materials{};
		std::vector<Mesh> meshes{};
		std::vector<Node> nodes{};
		std::vector<Sampler> samplers{};
		int scene{}; //base scene number
		std::vector<Scene> scenes{};
		std::vector<Texture> textures{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
			ModelJson, accessors, buffers, bufferViews, images,
			materials, meshes, nodes, samplers, scene, scenes, textures);
	};
}
