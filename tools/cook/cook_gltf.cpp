#include "cook_gltf.hpp"

#include "weavebound/asset/format.hpp"

#include <fstream>
#include <vector>

#if WEAVEBOUND_WITH_TINYGLTF
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#endif

namespace weavebound::cook_gltf_detail {

#if WEAVEBOUND_WITH_TINYGLTF

static const std::uint8_t* buffer_view_ptr(const tinygltf::Model& model, int view_idx, std::size_t& out_len) {
  out_len = 0;
  if (view_idx < 0 || view_idx >= static_cast<int>(model.bufferViews.size())) {
    return nullptr;
  }
  const tinygltf::BufferView& bv = model.bufferViews[static_cast<std::size_t>(view_idx)];
  if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
    return nullptr;
  }
  const tinygltf::Buffer& buf = model.buffers[static_cast<std::size_t>(bv.buffer)];
  if (bv.byteOffset + bv.byteLength > buf.data.size()) {
    return nullptr;
  }
  out_len = static_cast<std::size_t>(bv.byteLength);
  return buf.data.data() + bv.byteOffset;
}

static bool read_vec3_accessor(const tinygltf::Model& model, int accessor_idx, std::vector<std::array<float, 3>>& out) {
  out.clear();
  if (accessor_idx < 0 || accessor_idx >= static_cast<int>(model.accessors.size())) {
    return false;
  }
  const tinygltf::Accessor& acc = model.accessors[static_cast<std::size_t>(accessor_idx)];
  if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || acc.type != TINYGLTF_TYPE_VEC3) {
    return false;
  }
  std::size_t bv_len = 0;
  const std::uint8_t* base = buffer_view_ptr(model, acc.bufferView, bv_len);
  if (!base || acc.byteOffset >= bv_len) {
    return false;
  }
  if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) {
    return false;
  }
  const std::size_t stride = acc.ByteStride(model.bufferViews[static_cast<std::size_t>(acc.bufferView)]);
  const std::size_t count = static_cast<std::size_t>(acc.count);
  if (stride < sizeof(float) * 3 || acc.byteOffset + stride * count > bv_len) {
    return false;
  }
  out.resize(count);
  for (std::size_t i = 0; i < count; ++i) {
    const float* p = reinterpret_cast<const float*>(base + acc.byteOffset + stride * i);
    out[i] = {p[0], p[1], p[2]};
  }
  return true;
}

static bool read_vec2_accessor(const tinygltf::Model& model, int accessor_idx, std::vector<std::array<float, 2>>& out) {
  out.clear();
  if (accessor_idx < 0 || accessor_idx >= static_cast<int>(model.accessors.size())) {
    return false;
  }
  const tinygltf::Accessor& acc = model.accessors[static_cast<std::size_t>(accessor_idx)];
  if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || acc.type != TINYGLTF_TYPE_VEC2) {
    return false;
  }
  std::size_t bv_len = 0;
  const std::uint8_t* base = buffer_view_ptr(model, acc.bufferView, bv_len);
  if (!base || acc.byteOffset >= bv_len) {
    return false;
  }
  if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) {
    return false;
  }
  const std::size_t stride = acc.ByteStride(model.bufferViews[static_cast<std::size_t>(acc.bufferView)]);
  const std::size_t count = static_cast<std::size_t>(acc.count);
  if (stride < sizeof(float) * 2 || acc.byteOffset + stride * count > bv_len) {
    return false;
  }
  out.resize(count);
  for (std::size_t i = 0; i < count; ++i) {
    const float* p = reinterpret_cast<const float*>(base + acc.byteOffset + stride * i);
    out[i] = {p[0], p[1]};
  }
  return true;
}

#endif

}  // namespace weavebound::cook_gltf_detail

bool cook_glb_to_wbmesh(const char* glb_path, const char* out_wbmesh, std::string& err_msg) {
  err_msg.clear();
#if !WEAVEBOUND_WITH_TINYGLTF
  (void)glb_path;
  (void)out_wbmesh;
  err_msg = "cook built without WEAVEBOUND_WITH_TINYGLTF";
  return false;
#else
  using namespace weavebound::cook_gltf_detail;
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  if (!loader.LoadBinaryFromFile(&model, &err, &warn, glb_path)) {
    err_msg = err.empty() ? "LoadBinaryFromFile failed" : err;
    return false;
  }
  if (model.meshes.empty()) {
    err_msg = "no meshes";
    return false;
  }
  const tinygltf::Mesh& mesh = model.meshes[0];
  if (mesh.primitives.empty()) {
    err_msg = "no primitives";
    return false;
  }
  const tinygltf::Primitive& prim = mesh.primitives[0];
  if (prim.mode != TINYGLTF_MODE_TRIANGLES && prim.mode != -1) {
    err_msg = "primitive mode not triangles";
    return false;
  }

  auto pit = prim.attributes.find("POSITION");
  if (pit == prim.attributes.end()) {
    err_msg = "no POSITION";
    return false;
  }

  std::vector<std::array<float, 3>> pos;
  if (!read_vec3_accessor(model, pit->second, pos) || pos.empty()) {
    err_msg = "POSITION accessor";
    return false;
  }

  std::vector<std::array<float, 3>> nrm(pos.size(), std::array<float, 3>{0.f, 1.f, 0.f});
  auto nit = prim.attributes.find("NORMAL");
  if (nit != prim.attributes.end()) {
    std::vector<std::array<float, 3>> tmp;
    if (read_vec3_accessor(model, nit->second, tmp) && tmp.size() == pos.size()) {
      nrm = std::move(tmp);
    }
  }

  std::vector<std::array<float, 2>> uv(pos.size(), std::array<float, 2>{0.f, 0.f});
  auto uit = prim.attributes.find("TEXCOORD_0");
  if (uit == prim.attributes.end()) {
    uit = prim.attributes.find("TEXCOORD0");
  }
  if (uit != prim.attributes.end()) {
    std::vector<std::array<float, 2>> tmp;
    if (read_vec2_accessor(model, uit->second, tmp) && tmp.size() == pos.size()) {
      uv = std::move(tmp);
    }
  }

  constexpr std::uint32_t k_stride = 32;
  std::vector<float> vtx(pos.size() * (k_stride / sizeof(float)));
  for (std::size_t i = 0; i < pos.size(); ++i) {
    float* d = vtx.data() + i * (k_stride / sizeof(float));
    d[0] = pos[i][0];
    d[1] = pos[i][1];
    d[2] = pos[i][2];
    d[3] = nrm[i][0];
    d[4] = nrm[i][1];
    d[5] = nrm[i][2];
    d[6] = uv[i][0];
    d[7] = uv[i][1];
  }

  std::vector<std::uint8_t> idx_bytes;
  std::uint32_t index_count = 0;
  weavebound::asset::WbmeshHeader h{};
  h.vertex_count = static_cast<std::uint32_t>(pos.size());
  h.vertex_stride = k_stride;
  h.flags = 0;

  if (prim.indices < 0) {
    index_count = h.vertex_count;
    h.index_count = index_count;
    if (index_count > 65535u) {
      h.flags |= 1u;
      idx_bytes.resize(static_cast<std::size_t>(index_count) * 4u);
      auto* p = reinterpret_cast<std::uint32_t*>(idx_bytes.data());
      for (std::uint32_t i = 0; i < index_count; ++i) {
        p[i] = i;
      }
    } else {
      idx_bytes.resize(static_cast<std::size_t>(index_count) * 2u);
      auto* p = reinterpret_cast<std::uint16_t*>(idx_bytes.data());
      for (std::uint32_t i = 0; i < index_count; ++i) {
        p[i] = static_cast<std::uint16_t>(i);
      }
    }
  } else {
    const tinygltf::Accessor& iacc = model.accessors[static_cast<std::size_t>(prim.indices)];
    index_count = static_cast<std::uint32_t>(iacc.count);
    h.index_count = index_count;
    std::size_t bv_len = 0;
    const std::uint8_t* base = buffer_view_ptr(model, iacc.bufferView, bv_len);
    if (!base || iacc.byteOffset >= bv_len) {
      err_msg = "index buffer";
      return false;
    }
    if (iacc.bufferView < 0 || iacc.bufferView >= static_cast<int>(model.bufferViews.size())) {
      err_msg = "index bufferView";
      return false;
    }
    const std::size_t stride = iacc.ByteStride(model.bufferViews[static_cast<std::size_t>(iacc.bufferView)]);
    if (iacc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      if (stride < 2 || iacc.byteOffset + stride * index_count > bv_len) {
        err_msg = "index u16 range";
        return false;
      }
      idx_bytes.resize(static_cast<std::size_t>(index_count) * 2u);
      for (std::uint32_t i = 0; i < index_count; ++i) {
        const auto* s = reinterpret_cast<const std::uint16_t*>(base + iacc.byteOffset + stride * i);
        reinterpret_cast<std::uint16_t*>(idx_bytes.data())[i] = *s;
      }
    } else if (iacc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      h.flags |= 1u;
      if (stride < 4 || iacc.byteOffset + stride * index_count > bv_len) {
        err_msg = "index u32 range";
        return false;
      }
      idx_bytes.resize(static_cast<std::size_t>(index_count) * 4u);
      for (std::uint32_t i = 0; i < index_count; ++i) {
        const auto* s = reinterpret_cast<const std::uint32_t*>(base + iacc.byteOffset + stride * i);
        reinterpret_cast<std::uint32_t*>(idx_bytes.data())[i] = *s;
      }
    } else {
      err_msg = "unsupported index component type";
      return false;
    }
  }

  std::ofstream out(out_wbmesh, std::ios::binary);
  if (!out) {
    err_msg = "open output";
    return false;
  }
  out.write(reinterpret_cast<const char*>(&h), sizeof(h));
  out.write(reinterpret_cast<const char*>(vtx.data()), static_cast<std::streamsize>(vtx.size() * sizeof(float)));
  out.write(reinterpret_cast<const char*>(idx_bytes.data()), static_cast<std::streamsize>(idx_bytes.size()));
  return out.good();
#endif
}

bool write_white_texture_rgba8(const char* out_path) {
  weavebound::asset::WbtextureHeader h{};
  h.width_px = 1;
  h.height_px = 1;
  h.format = 0;
  h.mip_count = 1;
  std::uint32_t rgba = 0xffffffffu;
  std::ofstream out(out_path, std::ios::binary);
  if (!out) {
    return false;
  }
  out.write(reinterpret_cast<const char*>(&h), sizeof(h));
  out.write(reinterpret_cast<const char*>(&rgba), sizeof(rgba));
  return out.good();
}
