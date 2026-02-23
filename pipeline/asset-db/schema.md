# Asset Database Schema

Core entities:
- AssetRecord(guid, type, source, version, hash)
- DependencyEdge(fromGuid, toGuid, relationType)
- BuildArtifact(assetGuid, platform, artifactHash, createdAt)
