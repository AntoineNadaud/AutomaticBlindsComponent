import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, cover
from esphome.const import CONF_ID

DEPENDENCIES = ['i2c']

blind_controller_ns = cg.esphome_ns.namespace('blind_controller')
BlindControllerComponent = blind_controller_ns.class_('BlindControllerComponent', cover.Cover, cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = cover._COVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(BlindControllerComponent),
    cv.Optional("speed", default=50): cv.int_range(min=0, max=63),
    cv.Optional("auto_assign_address", default=True): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x09))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await cover.register_cover(var, config)
    
    cg.add(var.set_speed(config["speed"]))
    cg.add(var.set_auto_assign_address(config["auto_assign_address"]))
