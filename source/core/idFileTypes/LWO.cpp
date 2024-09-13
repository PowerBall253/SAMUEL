#include "LWO.h"

namespace HAYDEN
{
    void LWO_HEADER::ReadBinaryHeader(const std::vector<uint8_t> binaryData, bool hasEmbeddedGeo)
    {
        int offset = 0;

        // Read number of meshes
        Metadata = *(LWO_METADATA*)(binaryData.data() + offset);
        MeshInfo.resize(Metadata.NumMeshes);

        // Read LOD Deviations
        offset += sizeof(LWO_METADATA);
        MaxLODDeviations.resize(Metadata.NumLODs);

        for (int i = 0; i < Metadata.NumLODs; i++)
        {
            MaxLODDeviations[i] = *(uint32_t*)(binaryData.data() + offset);
            offset += sizeof(uint32_t);
        }

        IsStreamable = *(uint32_t*)(binaryData.data() + offset);
        offset += sizeof(uint32_t);

        // Read mesh and material info
        for (int i = 0; i < Metadata.NumMeshes; i++)
        {
            MeshInfo[i].MeshHeader = *(LWO_MESH_HEADER*)(binaryData.data() + offset);
            offset += sizeof(LWO_MESH_HEADER);

            int strLen = MeshInfo[i].MeshHeader.DeclStrlen;
            if (strLen < 0 || strLen > 1024)
            {
                // ADD ERROR HANDLER
                return;
            }

            MeshInfo[i].MaterialDeclName.resize(strLen);
            MeshInfo[i].MaterialDeclName = std::string(binaryData.data() + offset, binaryData.data() + offset + strLen);
            offset += strLen;

            MeshInfo[i].MeshFooter = *(LWO_MESH_FOOTER*)(binaryData.data() + offset);
            offset += sizeof(LWO_MESH_FOOTER);

            // Read LOD info for each mesh
            _BMLCount = Metadata.NumLODs;
            MeshInfo[i].LODInfo.resize(Metadata.NumLODs);
            for (int j = 0; j < Metadata.NumLODs; j++)
            {
                uint32_t absent = *(uint32_t*)(binaryData.data() + offset);
                offset += sizeof(uint32_t);

                if (absent == 0)
                {
                    MeshInfo[i].LODInfo[j] = *(LWO_LOD_INFO*)(binaryData.data() + offset);
                    offset += sizeof(LWO_LOD_INFO);
                }
            }
        }

        // Read model settings
        ModelSettings = *(LWO_MODEL_SETTINGS*)(binaryData.data() + offset);
        offset += sizeof(LWO_MODEL_SETTINGS);

        // Read texture axes
        TextureAxes.resize(ModelSettings.NumTextureAxes);
        for (int i = 0; i < ModelSettings.NumTextureAxes; i++)
        {
            TextureAxes[i] = *(TextureAxis*)(binaryData.data() + offset);
            offset += sizeof(TextureAxis);
        }

        // Read geo decals
        NumGeoDecals = *(uint32_t*)(binaryData.data() + offset);
        Projections.resize(NumGeoDecals);
        offset += sizeof(uint32_t);

        if (NumGeoDecals > 0)
        {
            // Read projections
            for (int i = 0; i < NumGeoDecals; i++)
            {
                Projections[i] = *(ModelGeoDecalProjection*)(binaryData.data() + offset);
                offset += sizeof(ModelGeoDecalProjection);
            }
        }

        // Read geodecal decl name
        uint32_t strLen = *(uint32_t*)(binaryData.data() + offset);
        offset += sizeof(uint32_t);
        GeoDecalDeclName.resize(strLen);
        GeoDecalDeclName = std::string(binaryData.data() + offset, binaryData.data() + offset + strLen);
        offset += strLen;

        // Read geodecal tint start offset
        GeoDecalTintStartOffset = *(uint32_t*)(binaryData.data() + offset);
        offset += sizeof(uint32_t);

        // Read streamed surfaces
        uint32_t streamedSurfacesCount = Metadata.NumLODs * Metadata.NumMeshes;
        StreamedSurfaces.resize(streamedSurfacesCount);
        for (int i = 0; i < streamedSurfacesCount; i++)
        {
            StreamedSurfaces[i] = *(uint8_t*)(binaryData.data() + offset);
            offset += sizeof(uint8_t);
        }

        // Get embedded model geometry if needed
        if (hasEmbeddedGeo)
        {
            EmbeddedGeo.insert(EmbeddedGeo.begin(), binaryData.begin() + offset, binaryData.end());
        }
        else if (IsStreamable)
        {
            const uint64_t streamDBStructCount = 5;
            StreamDBHeaders.resize(streamDBStructCount);
            StreamDiskLayout.resize(streamDBStructCount);
            uint64_t streamDBStructSize = sizeof(LWO_STREAMDB_HEADER) + sizeof(LWO_GEOMETRY_STREAMDISK_LAYOUT);

            if (MeshInfo[0].LODInfo[0].GeoFlags.Flags1 == 60)
            {
                streamDBStructSize += sizeof(LWO_STREAMDB_DATA);
                StreamDBData.resize(streamDBStructCount);
            }
            else
            {
                streamDBStructSize += sizeof(LWO_STREAMDB_DATA_VARIANT);
                StreamDBDataVariant.resize(streamDBStructCount);
            }

            // Read StreamDB info
            for (int i = 0; i < streamDBStructCount; i++)
            {
                StreamDBHeaders[i] = *(LWO_STREAMDB_HEADER*)(binaryData.data() + offset);
                offset += sizeof(LWO_STREAMDB_HEADER);

                if (MeshInfo[0].LODInfo[0].GeoFlags.Flags1 == 60)
                {
                    StreamDBData[i] = *(LWO_STREAMDB_DATA*)(binaryData.data() + offset);
                    offset += sizeof(LWO_STREAMDB_DATA);
                }
                else
                {
                    StreamDBDataVariant[i] = *(LWO_STREAMDB_DATA_VARIANT*)(binaryData.data() + offset);
                    offset += sizeof(LWO_STREAMDB_DATA_VARIANT);
                }

                StreamDiskLayout[i] = *(LWO_GEOMETRY_STREAMDISK_LAYOUT*)(binaryData.data() + offset);
                offset += sizeof(LWO_GEOMETRY_STREAMDISK_LAYOUT);
            }

            // Abort export in this case, we can't handle this yet.
            if (StreamDBHeaders[0].NumStreams > 1)
            {
                return;
            }
        }

        // Temporary for rev-eng
        RawLWOHeader.insert(RawLWOHeader.begin(), binaryData.begin(), binaryData.end());
        return;
    }

    void LWO::SerializeEmbeddedGeo(LWO_HEADER lwoHeader, std::vector<uint8_t> lwoGeo)
    {
        Header = lwoHeader;
        MeshGeometry.resize(Header.Metadata.NumMeshes);

        // Allocate
        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            MeshGeometry[i].Vertices.resize(Header.MeshInfo[i].LODInfo[0].NumVertices);
            MeshGeometry[i].Normals.resize(Header.MeshInfo[i].LODInfo[0].NumVertices);
            MeshGeometry[i].UVs.resize(Header.MeshInfo[i].LODInfo[0].NumVertices);
            MeshGeometry[i].Faces.resize(Header.MeshInfo[i].LODInfo[0].NumEdges / 3);
        }

        // World geo is stored sequentially. We can get by with just 1 offset, update as we go
        uint64_t offset = 0;

        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            for (int j = 0; j < MeshGeometry[i].Vertices.size(); j++)
            {
                // Read Vertices
                MeshGeometry[i].Vertices[j] = *(Vertex*)(lwoGeo.data() + offset);
                offset += sizeof(Vertex);

                // Read UVs
                MeshGeometry[i].UVs[j] = *(UV*)(lwoGeo.data() + offset);
                offset += sizeof(UV);

                // Read Normals + Tangents
                PackedNormal packedNormal = *(PackedNormal*)(lwoGeo.data() + offset);
                MeshGeometry[i].Normals[j] = MeshGeometry[i].UnpackNormal(packedNormal);
                offset += sizeof(PackedNormal);

                // Skip Colors
                offset += sizeof(uint32_t);

                // Skip Lightmap UVs (world brushes only)
                if (Header.ModelSettings.LightmapSurfaceAreaSqrt != 0)
                {
                    offset += sizeof(uint64_t);
                }
            }

            // Read Faces
            for (int j = 0; j < MeshGeometry[i].Faces.size(); j++)
            {
                MeshGeometry[i].Faces[j] = *(Face*)(lwoGeo.data() + offset);
                offset += sizeof(Face);
            }
        }
    }

    void LWO::SerializeStreamedGeo(LWO_HEADER lwoHeader, std::vector<uint8_t> lwoGeo)
    {
        Header = lwoHeader;
        MeshGeometry.resize(Header.Metadata.NumMeshes);

        uint64_t offset = 0;
        uint64_t offsetNormals = 0;
        uint64_t offsetUVs = 0;
        uint64_t offsetFaces = 0;

        // Get offsets from LWO streamdb data
        if (Header.MeshInfo[0].LODInfo[0].GeoFlags.Flags1 == 60)
        {
            offsetNormals = Header.StreamDBData[0].NormalStartOffset;
            offsetUVs = Header.StreamDBData[0].UVStartOffset;
            offsetFaces = Header.StreamDBData[0].FacesStartOffset;
        }
        else
        {
            offsetNormals = Header.StreamDBDataVariant[0].NormalStartOffset;
            offsetUVs = Header.StreamDBDataVariant[0].UVStartOffset;
            offsetFaces = Header.StreamDBDataVariant[0].FacesStartOffset;
        }

        // Allocate
        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            MeshGeometry[i].Vertices.resize(Header.MeshInfo[i].LODInfo[0].NumVertices);
            MeshGeometry[i].Normals.resize(Header.MeshInfo[i].LODInfo[0].NumVertices);
            MeshGeometry[i].UVs.resize(Header.MeshInfo[i].LODInfo[0].NumVertices);
            MeshGeometry[i].Faces.resize(Header.MeshInfo[i].LODInfo[0].NumEdges / 3);
        }

        // Read Vertices
        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            for (int j = 0; j < MeshGeometry[i].Vertices.size(); j++)
            {
                PackedVertex packedVertex = *(PackedVertex*)(lwoGeo.data() + offset);
                MeshGeometry[i].Vertices[j] = MeshGeometry[i].UnpackVertex(packedVertex, Header.MeshInfo[i].LODInfo[0].GeoMeta.VertexOffsetX, Header.MeshInfo[i].LODInfo[0].GeoMeta.VertexOffsetY, Header.MeshInfo[i].LODInfo[0].GeoMeta.VertexOffsetZ, Header.MeshInfo[i].LODInfo[0].GeoMeta.VertexScale);
                offset += sizeof(PackedVertex);
            }
        }
     
        // Read Normals
        offset = offsetNormals;
        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            for (int j = 0; j < MeshGeometry[i].Normals.size(); j++)
            {
                PackedNormal packedNormal = *(PackedNormal*)(lwoGeo.data() + offset);
                MeshGeometry[i].Normals[j] = MeshGeometry[i].UnpackNormal(packedNormal);
                offset += sizeof(PackedNormal);
            }
        }

        // Read UVs
        offset = offsetUVs;
        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            for (int j = 0; j < MeshGeometry[i].UVs.size(); j++)
            {
                PackedUV packedUV = *(PackedUV*)(lwoGeo.data() + offset);
                MeshGeometry[i].UVs[j] = MeshGeometry[i].UnpackUV(packedUV, Header.MeshInfo[i].LODInfo[0].GeoMeta.UVMapOffsetU, Header.MeshInfo[i].LODInfo[0].GeoMeta.UVMapOffsetV, Header.MeshInfo[i].LODInfo[0].GeoMeta.UVScale);
                offset += sizeof(PackedUV);
            }
        }

        // Read Faces
        offset = offsetFaces;
        for (int i = 0; i < MeshGeometry.size(); i++)
        {
            for (int j = 0; j < MeshGeometry[i].Faces.size(); j++)
            {
                MeshGeometry[i].Faces[j] = *(Face*)(lwoGeo.data() + offset);
                offset += sizeof(Face);
            }
        }
        return;
    }
}