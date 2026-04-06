#include "weavebound/render_graph/builder.hpp"

#include <utility>

namespace weavebound::rg {

ResourceId RenderGraphBuilder::add_resource(std::string name, bool transient, ResourceSurfaceKind surface) {
  const ResourceId id = static_cast<ResourceId>(resources_.size());
  ResourceNode n{};
  n.id = id;
  n.name = std::move(name);
  n.transient = transient;
  n.surface = surface;
  resources_.push_back(std::move(n));
  return id;
}

PassId RenderGraphBuilder::add_pass(std::string name, PassKind kind) {
  const PassId id = static_cast<PassId>(passes_.size());
  passes_.push_back(PassNode{id, std::move(name), kind, {}, {}});
  return id;
}

void RenderGraphBuilder::pass_reads(PassId pass, ResourceId resource) {
  if (pass >= passes_.size()) {
    return;
  }
  passes_[pass].reads.push_back(resource);
}

void RenderGraphBuilder::pass_writes(PassId pass, ResourceId resource) {
  if (pass >= passes_.size()) {
    return;
  }
  passes_[pass].writes.push_back(resource);
}

}  // namespace weavebound::rg
