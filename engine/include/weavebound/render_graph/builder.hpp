#pragma once

#include "weavebound/render_graph/ir.hpp"

namespace weavebound::rg {

/** 建立 Pass / Resource 與讀寫邊（規格 1.3 IR 建置階段）。 */
class RenderGraphBuilder {
 public:
  RenderGraphBuilder() = default;

  ResourceId add_resource(std::string name, bool transient,
                          ResourceSurfaceKind surface = ResourceSurfaceKind::Color);
  PassId add_pass(std::string name, PassKind kind);

  void pass_reads(PassId pass, ResourceId resource);
  void pass_writes(PassId pass, ResourceId resource);

  const std::vector<ResourceNode>& resources() const { return resources_; }
  const std::vector<PassNode>& passes() const { return passes_; }

 private:
  std::vector<ResourceNode> resources_;
  std::vector<PassNode> passes_;
};

}  // namespace weavebound::rg
