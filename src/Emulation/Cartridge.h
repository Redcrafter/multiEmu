#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "Mappers/Mapper.h"

std::shared_ptr<Mapper> LoadCart(const std::string& path);
