#include <Quarisma/core/functional.h>
#include <Quarisma/core/jit_type.h>
#include <Quarisma/core/qualified_name.h>
#include <quarisma/util/hash.h>
#include <torch/csrc/jit/ir/ir.h>
#include <torch/csrc/jit/ir/type_hashing.h>

namespace torch::jit
{

namespace
{
size_t hashType(const Type& type)
{
    if (auto named_type = type.castRaw<ClassType>())
    {
        return quarisma::get_hash(named_type->name().value(), named_type->compilation_unit());
    }
    size_t hash = 0;
    for (const auto& containedType : type.containedTypes())
    {
        hash = quarisma::hash_combine(hash, hashType(*containedType));
    }
    hash = quarisma::hash_combine(hash, get_hash(type.kind()));
    return hash;
}
}  // namespace

size_t HashType::operator()(const TypePtr& type) const
{
    return hashType(*type);
}

size_t HashType::operator()(const quarisma::ConstTypePtr& type) const
{
    return hashType(*type);
}

bool EqualType::operator()(const TypePtr& a, const TypePtr& b) const
{
    return *a == *b;
}

bool EqualType::operator()(const quarisma::ConstTypePtr& a, const quarisma::ConstTypePtr& b) const
{
    return *a == *b;
}

}  // namespace torch::jit
