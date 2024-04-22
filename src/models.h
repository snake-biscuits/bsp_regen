#pragma once


struct mstudiopertrihdr_t
{
	short version; // game requires this to be 2 or else it errors

	short unk; // may or may not exist, version gets casted as short in ida

	Vector3 bbmin;
	Vector3 bbmax;

	int unused[8];
};

struct studiohdr_t
{
	int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
	int version; // Format version number, such as 48 (0x30,0x00,0x00,0x00)
	int checksum; // This has to be the same in the phy and vtx files to load!
	char name[64]; // The internal name of the model, padding with null bytes.
	// Typically "my_model.mdl" will have an internal name of "my_model"
	int length; // Data size of MDL file in bytes.

	Vector3 eyeposition;	// ideal eye position

	Vector3 illumposition;	// illumination center

	Vector3 hull_min;		// ideal movement hull size
	Vector3 hull_max;			

	Vector3 view_bbmin;		// clipping bounding box
	Vector3 view_bbmax;		

	int flags;

	// highest observed: 250
	int numbones; // bones
	int boneindex;

	int numbonecontrollers; // bone controllers
	int bonecontrollerindex;

	int numhitboxsets;
	int hitboxsetindex;

	int numlocalanim; // animations/poses
	int localanimindex; // animation descriptions

	int numlocalseq; // sequences
	int	localseqindex;

	int activitylistversion; // initialization flag - have the sequences been indexed?
	int eventsindexed;

	// raw textures
	int numtextures;
	int textureindex;

	/// raw textures search paths
	int numcdtextures;
	int cdtextureindex;

	// replaceable textures tables
	int numskinref;
	int numskinfamilies;
	int skinindex;

	int numbodyparts;		
	int bodypartindex;

	int numlocalattachments;
	int localattachmentindex;

	int numlocalnodes;
	int localnodeindex;
	int localnodenameindex;

	int deprecated_numflexdesc;
	int deprecated_flexdescindex;

	int deprecated_numflexcontrollers;
	int deprecated_flexcontrollerindex;

	int deprecated_numflexrules;
	int deprecated_flexruleindex;

	int numikchains;
	int ikchainindex;

	int deprecated_nummouths;
	int deprecated_mouthindex;

	int numlocalposeparameters;
	int localposeparamindex;

	int surfacepropindex;

	int keyvalueindex;
	int keyvaluesize;

	int numlocalikautoplaylocks;
	int localikautoplaylockindex;


	float mass;
	uint32_t contents;

	// external animations, models, etc.
	int numincludemodels;
	int includemodelindex;

	// implementation specific back pointer to virtual data
	int /* mutable void* */ virtualModel;

	// for demand loaded animation blocks
	int szanimblocknameindex;

	int numanimblocks;
	int animblockindex;

	int /* mutable void* */ animblockModel;

	int bonetablebynameindex;

	// used by tools only that don't cache, but persist mdl's peer data
	// engine uses virtualModel to back link to cache pointers
	int /* void* */ pVertexBase;
	int /* void* */ pIndexBase;

	// if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
	// this value is used to calculate directional components of lighting 
	// on static props
	BYTE constdirectionallightdot;

	// set during load of mdl data to track *desired* lod configuration (not actual)
	// the *actual* clamped root lod is found in studiohwdata
	// this is stored here as a global store to ensure the staged loading matches the rendering
	BYTE rootLOD;

	// set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
	// to be set as root LOD:
	//	numAllowedRootLODs = 0	means no restriction, any lod can be set as root lod.
	//	numAllowedRootLODs = N	means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
	BYTE numAllowedRootLODs;

	BYTE unused;

	float fadeDistance;

	int deprecated_numflexcontrollerui;
	int deprecated_flexcontrolleruiindex;

	float flVertAnimFixedPointScale;
	int surfacepropLookup;	// this index must be cached by the loader, not saved in the file

	// NOTE: No room to add stuff? Up the .mdl file format version 
	// [and move all fields in studiohdr2_t into studiohdr_t and kill studiohdr2_t],
	// or add your stuff to studiohdr2_t. See NumSrcBoneTransforms/SrcBoneTransform for the pattern to use.
	int studiohdr2index;

	int sourceFilenameOffset; // in v52 not every model has these strings, only four bytes when not present.
};

struct studiohdr2_t
{
	int numsrcbonetransform;
	int srcbonetransformindex;

	int	illumpositionattachmentindex;

	float flMaxEyeDeflection; // default to cos(30) if not set

	int linearboneindex;

	int sznameindex;

	int m_nBoneFlexDriverCount;
	int m_nBoneFlexDriverIndex;

	// for static props (and maybe others)
	// Precomputed Per-Triangle AABB data
	int m_nPerTriAABBIndex;
	int m_nPerTriAABBNodeCount;
	int m_nPerTriAABBLeafCount;
	int m_nPerTriAABBVertCount;

	// always "" or "Titan"
	int unkstringindex;

	int reserved[39];
};

class Model {
	memory_mapped_file file_;
	studiohdr_t* header_;
	studiohdr2_t* header2_;
public:
	Model(const char* fileName) {
		if (!file_.open_existing(fileName)) {
			char buffer[1024];
			snprintf(buffer,1024,"Failed to open file %s",fileName);
			throw std::runtime_error(buffer);
		}
		header_ = file_.rawdata<studiohdr_t>();
		header2_ = file_.rawdata<studiohdr2_t>(header_->studiohdr2index);
	}
	~Model() {
		file_.close();
	}

	mstudiopertrihdr_t* getPerTriHeader() {
		if(header2_->m_nPerTriAABBIndex == 0)return 0;
		return file_.rawdata<mstudiopertrihdr_t>(header_->studiohdr2index+header2_->m_nPerTriAABBIndex);
	}
	uint32_t getContents(){
		return header_->contents;
	}
};