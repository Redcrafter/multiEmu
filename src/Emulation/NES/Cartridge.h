#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Mappers/Mapper.h"

namespace Nes {

void LoadCardDb(const std::string& path);
std::shared_ptr<Mapper> LoadCart(const std::string& path);

}
