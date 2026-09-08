#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "trace.h"
#include <stdint.h>

#define TYPE_LM95241 "lm95241"
OBJECT_DECLARE_SIMPLE_TYPE(LM95241State, LM95241)

struct LM95241State {
  /*< private >*/
  I2CSlave i2c;
  /*< public >*/

  uint8_t pointer;
  uint8_t length;

  int32_t local_temperature;
  int32_t remote1_temperature;
  int32_t remote2_temperature;
};

// TODO: extend temperature sesnors to local, remote1, and remote2
static void lm95241_get_temperature(Object *obj, Visitor *v, const char *name,
                                    void *opaque, Error **errp) {
  LM95241State *s = LM95241(obj);
  int64_t value = s->local_temperature;

  visit_type_int(v, name, &value, errp);
}

static void lm95241_set_temperature(Object *obj, Visitor *v, const char *name,
                                    void *opaque, Error **errp) {
  LM95241State *s = LM95241(obj);
  int64_t temp;

  if (!visit_type_int(v, name, &temp, errp)) {
    return;
  }

  s->local_temperature = (int32_t)temp;
}

static int lm95241_event(I2CSlave *i2c, enum i2c_event event) {
  LM95241State *s = LM95241(i2c);
  switch (event) {
  case I2C_START_SEND:
    s->length = 0;
    break;
  case I2C_START_RECV:
    s->length = 0;
    break;
  case I2C_FINISH:
    s->length = 0;
    break;
  default:
    break;
  }
  return 0; /* ACK */
}

static uint8_t lm95241_recv(I2CSlave *i2c) {
  LM95241State *s = LM95241(i2c);
  uint8_t res = 0;

  switch (s->pointer) {
    /* Local Temperature MSB */
  case 0x10:
    res = (int8_t)((s->local_temperature / 250) >> 2);
    break;
    /* Remote 1 Temperature MSB */
  case 0x11:
    res = (int8_t)((s->remote1_temperature / 250) >> 2);
    break;
    /* Remote 2 Temperature MSB */
  case 0x12:
    res = (int8_t)((s->remote2_temperature / 250) >> 2);
    break;
    /* Local Temperature LSB */
  case 0x20:
    res = (uint8_t)((s->local_temperature / 250) << 6);
    break;
    /* Remote Temperature LSB */
  case 0x21:
    res = (uint8_t)((s->remote1_temperature / 250) << 6);
    break;
    /* Remote Temperature LSB */
  case 0x22:
    res = (uint8_t)((s->remote2_temperature / 250) << 6);
    break;
  default:
    res = 0xFF;
    break;
  }
  s->length++;
  return res;
}

static int lm95241_send(I2CSlave *i2c, uint8_t data) {
  LM95241State *s = LM95241(i2c);

  s->pointer = data;

  s->length++;

  return 0; /* ACK */
}

static void lm95241_initfn(Object *obj) {
  LM95241State *s = LM95241(obj);

  s->local_temperature = 25000;
  s->remote1_temperature = 25500;
  s->remote2_temperature = 24500;

  object_property_add(obj, "local_temperature", "int", lm95241_get_temperature,
                      lm95241_set_temperature, NULL, NULL);
}

static void lm95241_class_init(ObjectClass *klass, const void *data) {
  DeviceClass *dc = DEVICE_CLASS(klass);
  I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

  k->event = lm95241_event;
  k->recv = lm95241_recv;
  k->send = lm95241_send;

  dc->vmsd = NULL;
  /* dc->vmsd = &vmstate_lm95241; */
}

static const TypeInfo lm95241_info = {
    .name = TYPE_LM95241,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(LM95241State),
    .instance_init = lm95241_initfn,
    .class_init = lm95241_class_init,
};

static void lm95241_register_types(void) {
  type_register_static(&lm95241_info);
}

type_init(lm95241_register_types)

    // #error "寫完後還沒對過tmp105" // FIXME:
