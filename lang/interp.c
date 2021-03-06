/*
MIT License

Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "err.h"
#include "val.h"
#include "bcode.h"
#include "parse.h"
#include "compile.h"
#include "interp.h"

#include "number.h"
#include "string.h"
#include "function.h"
#include "array.h"
#include "object.h"

static val_t undefined = TAG_UNDEFINED;

static inline void interp_neg(env_t *env) {
    val_t *v = env_stack_peek(env);

    if (val_is_number(v)) {
        return val_set_number(v, -val_2_double(v));
    } else {
        return val_set_nan(v);
    }
}

static inline void interp_not(env_t *env) {
    val_t *v = env_stack_peek(env);

    if (val_is_number(v)) {
        return val_set_number(v, ~val_2_integer(v));
    } else {
        return val_set_nan(v);
    }
}

static inline void interp_logic_not(env_t *env) {
    val_t *v = env_stack_peek(env);

    val_set_boolean(v, !val_is_true(v));
}

static inline void interp_mul(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_mul(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_div(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_div(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_mod(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_mod(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_add(env_t *env) {
    val_t *b = env_stack_peek(env); // Note: keep in stack, deffence GC!
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_add(env, a, b, res);
    } else
    if (val_is_string(a)){
        // Maybe cause gc here!
        string_add(env, a, b, res);
    } else {
        val_set_nan(res);
    }
    env_stack_pop(env);
}

static inline void interp_sub(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_sub(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_and(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_and(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_or(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_or(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_xor(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_xor(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_lshift(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_lshift(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline void interp_rshift(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        number_rshift(env, a, b, res);
    } else {
        val_set_nan(res);
    }
}

static inline int interp_test_equal(val_t *a, val_t *b) {
    if (*a == *b) {
        return !(val_is_nan(a) || val_is_undefined(a));
    } else {
        if (val_is_string(a)) {
            return string_compare(a, b) == 0;
        } else {
            return 0;
        }
    }
}

static inline void interp_teq(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    val_set_boolean(res, interp_test_equal(a, b));
}

static inline void interp_tne(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    val_set_boolean(res, !interp_test_equal(a, b));
}

static inline void interp_tgt(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        if (val_is_number(b)) {
            val_set_boolean(res, val_2_double(a) - val_2_double(b) > 0);
            return;
        }
    } else
    if (val_is_string(a)) {
        if (val_is_string(b)) {
            val_set_boolean(res, string_compare(a, b) > 0);
            return;
        }
    }
    val_set_boolean(res, 0);
}

static inline void interp_tge(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        if (val_is_number(b)) {
            val_set_boolean(res, val_2_double(a) - val_2_double(b) >= 0);
            return;
        }
    } else
    if (val_is_string(a)) {
        if (val_is_string(b)) {
            val_set_boolean(res, string_compare(a, b) >= 0);
            return;
        }
    }
    val_set_boolean(res, 0);
}

static inline void interp_tlt(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        if (val_is_number(b)) {
            val_set_boolean(res, val_2_double(a) - val_2_double(b) < 0);
            return;
        }
    } else
    if (val_is_string(a)) {
        if (val_is_string(b)) {
            val_set_boolean(res, string_compare(a, b) < 0);
            return;
        }
    }
    val_set_boolean(res, 0);
}

static inline void interp_tle(env_t *env) {
    val_t *b = env_stack_pop(env);
    val_t *a = b + 1;
    val_t *res = a;

    if (val_is_number(a)) {
        if (val_is_number(b)) {
            val_set_boolean(res, val_2_double(a) - val_2_double(b) <= 0);
            return;
        }
    } else
    if (val_is_string(a)) {
        if (val_is_string(b)) {
            val_set_boolean(res, string_compare(a, b) <= 0);
            return;
        }
    }
    val_set_boolean(res, 0);
}

static inline void interp_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft) {
            *res = *lft = *rht;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_add_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft) {
            if (val_is_number(lft)) {
                number_add(env, lft, rht, lft);
            } else
            if (val_is_string(lft)){
                string_add(env, lft, rht, lft);
            } else {
                val_set_nan(lft);
            }

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }

    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_sub_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_sub(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_mul_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_mul(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_div_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_div(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_mod_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_mod(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_and_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_and(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_or_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_or(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_xor_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_xor(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_lshift_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_lshift(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline void interp_rshift_assign(env_t *env) {
    val_t *rht = env_stack_peek(env);
    val_t *lft = rht + 1;

    if (val_is_reference(lft)) {
        val_t *res = lft;
        uint8_t id, generation;

        val_2_reference(lft, &id, &generation);
        lft = env_get_var(env, id, generation);
        if (lft && val_is_number(lft)) {
            number_rshift(env, lft, rht, lft);

            *res = *lft;
            env_stack_pop(env);
            return;
        }
    }
    env_set_error(env, ERR_InvalidLeftValue);
}

static inline const uint8_t *interp_call(env_t *env, int ac, const uint8_t *pc) {
    val_t *fn = env_stack_peek(env);
    val_t *av = fn + 1;

    if (val_is_script(fn)) {
        return env_frame_setup(env, pc, fn, ac, av);
    } else
    if (val_is_native(fn)) {
        env_native_call(env, fn, ac, av);
    } else {
        env_set_error(env, ERR_InvalidCallor);
    }
    return pc;
}

static inline void interp_array(env_t *env, int n) {
    val_t *av = env_stack_peek(env);
    intptr_t array = array_create(env, n, av);

    if (array) {
        val_set_array(env_stack_release(env, n - 1), array);
    } else {
        val_set_undefined(env_stack_release(env, n - 1));
    }
}

static inline void interp_dict(env_t *env, int n) {
    val_t *av = env_stack_peek(env);
    intptr_t dict = object_create(env, n, av);

    if (dict) {
        val_set_dictionary(env_stack_release(env, n - 1), dict);
    } else {
        val_set_undefined(env_stack_release(env, n - 1));
    }
}

static inline void interp_prop_self(env_t *env) {
    val_t *key = env_stack_peek(env);
    val_t *self = key + 1;
    val_t *prop = key;

    object_prop_get(env, self, key, prop);
    // no pop
}

static inline void interp_prop_get(env_t *env) {
    val_t *key = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_get(env, obj, key, res);
    env_stack_pop(env);
}

static inline void interp_elem_self(env_t *env) {
    val_t *key = env_stack_peek(env); // keey the "key" in stack
    val_t *obj = key + 1;
    val_t *res = key;

    object_elem_get(env, obj, key, res);
}

static inline void interp_elem_get(env_t *env) {
    val_t *key = env_stack_peek(env); // keey the "key" in stack
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_get(env, obj, key, res);
    env_stack_pop(env);
}

static inline void interp_prop_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_set(env, obj, key, val);
    env_stack_release(env, 2);
    *res = *val;
}

static inline void interp_elem_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_set(env, obj, key, val);
    env_stack_release(env, 2);
    *res = *val;
}

static inline void interp_prop_add_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_add_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_add_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_add_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_sub_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_sub_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_sub_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_sub_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_mul_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_mul_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_mul_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_mul_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_div_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_div_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_div_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_div_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_mod_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_mod_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_mod_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_mod_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_and_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_and_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_and_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_and_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_or_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_or_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_or_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_or_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_xor_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_xor_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_xor_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_xor_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_lshift_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_lshift_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_lshift_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_lshift_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_prop_rshift_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_prop_rshift_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline void interp_elem_rshift_set(env_t *env) {
    val_t *val = env_stack_peek(env); // keep the "key" in stack, defence GC
    val_t *key = val + 1;
    val_t *obj = key + 1;
    val_t *res = obj;

    object_elem_rshift_set(env, obj, key, val, res);
    env_stack_release(env, 2);
}

static inline
void interp_push_function(env_t *env, unsigned int id)
{
    uint8_t *entry;
    intptr_t fn;

    if (id >= env->exe.func_num) {
        env_set_error(env, ERR_SysError);
        return;
    }

    entry = env->exe.func_map[id];
    fn = function_create(env, entry);
    if (0 == fn) {
        env_set_error(env, ERR_SysError);
    } else {
        env_push_script(env, fn);
    }
}

#if 0
#define __INTERP_SHOW__
static inline void interp_show(const uint8_t *pc, int sp) {
    const char *cmd;
    int param1, param2, n;

    n = bcode_parse(pc, NULL, &cmd, &param1, &param2);

    if (n == 0) {
        printf("[PC: %p, SP: %d] %s\n", pc, sp, cmd);
    } else
    if (n == 1) {
        printf("[PC: %p, SP: %d] %s %d\n", pc, sp, cmd, param1);
    } else {
        printf("[PC: %p, SP: %d] %s %d %d\n", pc, sp, cmd, param1, param2);
    }
}
#endif

static int interp_run(env_t *env, const uint8_t *pc)
{
    int     index;

    while(!env->error) {
        uint8_t code;

#if defined(__INTERP_SHOW__)
        interp_show(pc, env->sp);
#endif
        code = *pc++;
        switch(code) {
        case BC_STOP:       goto DO_END;
        case BC_PASS:       break;

        /* Return instruction */
        case BC_RET0:       env_frame_restore(env, &pc, &env->scope);
                            env_push_undefined(env);
                            break;

        case BC_RET:        {
                                val_t *res = env_stack_peek(env);
                                env_frame_restore(env, &pc, &env->scope);
                                *env_stack_push(env) = *res;
                            }
                            break;

        /* Jump instruction */
        case BC_SJMP:       index = (int8_t) (*pc++); pc += index;
                            break;

        case BC_JMP:        index = (int8_t) (*pc++); index = (index << 8) | (*pc++); pc += index;
                            break;

        case BC_SJMP_T:     index = (int8_t) (*pc++);
                            if (val_is_true(env_stack_peek(env))) {
                                pc += index;
                            }
                            break;

        case BC_SJMP_F:     index = (int8_t) (*pc++);
                            if (!val_is_true(env_stack_peek(env))) {
                                pc += index;
                            }
                            break;

        case BC_JMP_T:      index = (int8_t) (*pc++); index = (index << 8) | (*pc++);
                            if (val_is_true(env_stack_peek(env))) {
                                pc += index;
                            }
                            break;
        case BC_JMP_F:      index = (int8_t) (*pc++); index = (index << 8) | (*pc++);
                            if (!val_is_true(env_stack_peek(env))) {
                                pc += index;
                            }
                            break;
        case BC_POP_SJMP_T: index = (int8_t) (*pc++);
                            if (val_is_true(env_stack_pop(env))) {
                                pc += index;
                            }
                            break;
        case BC_POP_SJMP_F: index = (int8_t) (*pc++);
                            if (!val_is_true(env_stack_pop(env))) {
                                pc += index;
                            }
                            break;
        case BC_POP_JMP_T:  index = (int8_t) (*pc++); index = (index << 8) | (*pc++);
                            if (val_is_true(env_stack_pop(env))) {
                                pc += index;
                            }
                            break;
        case BC_POP_JMP_F:  index = (int8_t) (*pc++); index = (index << 8) | (*pc++);
                            if (!val_is_true(env_stack_pop(env))) {
                                pc += index;
                            }
                            break;

        case BC_PUSH_UND:   env_push_undefined(env);  break;
        case BC_PUSH_NAN:   env_push_nan(env);        break;
        case BC_PUSH_TRUE:  env_push_boolean(env, 1); break;
        case BC_PUSH_FALSE: env_push_boolean(env, 0); break;
        case BC_PUSH_ZERO:  env_push_zero(env);  break;

        case BC_PUSH_NUM:   index = (*pc++); index = (index << 8) + (*pc++);
                            env_push_number(env, index);
                            break;

        case BC_PUSH_STR:   index = (*pc++); index = (index << 8) + (*pc++);
                            env_push_string(env, index);
                            break;

        case BC_PUSH_VAR:   index = (*pc++); env_push_var(env, index, *pc++);
                            break;

        case BC_PUSH_REF:   index = (*pc++); env_push_ref(env, index, *pc++);
                            break;


        case BC_PUSH_SCRIPT:index = (*pc++); index = (index << 8) | (*pc++);
                            interp_push_function(env, index);
                            break;

        case BC_PUSH_NATIVE:index = (*pc++); index = (index << 8) | (*pc++);
                            env_push_native(env, index);
                            break;

        case BC_POP:        env_stack_pop(env); break;

        case BC_NEG:        interp_neg(env); break;
        case BC_NOT:        interp_not(env); break;
        case BC_LOGIC_NOT:  interp_logic_not(env); break;

        case BC_MUL:        interp_mul(env); break;
        case BC_DIV:        interp_div(env); break;
        case BC_MOD:        interp_mod(env); break;
        case BC_ADD:        interp_add(env); break;
        case BC_SUB:        interp_sub(env); break;

        case BC_AAND:       interp_and(env); break;
        case BC_AOR:        interp_or(env);  break;
        case BC_AXOR:       interp_xor(env); break;

        case BC_LSHIFT:     interp_lshift(env); break;
        case BC_RSHIFT:     interp_rshift(env); break;

        case BC_TEQ:        interp_teq(env); break;
        case BC_TNE:        interp_tne(env); break;
        case BC_TGT:        interp_tgt(env); break;
        case BC_TGE:        interp_tge(env); break;
        case BC_TLT:        interp_tlt(env); break;
        case BC_TLE:        interp_tle(env); break;

        case BC_TIN:        env_set_error(env, ERR_InvalidByteCode); break;

        case BC_PROP:       interp_prop_get(env);  break;
        case BC_PROP_METH:  interp_prop_self(env); break;
        case BC_ELEM:       interp_elem_get(env);  break;
        case BC_ELEM_METH:  interp_elem_self(env); break;

        case BC_ASSIGN:     interp_assign(env); break;
        case BC_ADD_ASSIGN: interp_add_assign(env); break;
        case BC_SUB_ASSIGN: interp_sub_assign(env); break;
        case BC_MUL_ASSIGN: interp_mul_assign(env); break;
        case BC_DIV_ASSIGN: interp_div_assign(env); break;
        case BC_MOD_ASSIGN: interp_mod_assign(env); break;
        case BC_AND_ASSIGN: interp_and_assign(env); break;
        case BC_OR_ASSIGN:  interp_or_assign(env); break;
        case BC_XOR_ASSIGN: interp_xor_assign(env); break;
        case BC_LSHIFT_ASSIGN:      interp_lshift_assign(env); break;
        case BC_RSHIFT_ASSIGN:      interp_rshift_assign(env); break;

        case BC_PROP_ASSIGN:        interp_prop_set(env); break;
        case BC_PROP_ADD_ASSIGN:    interp_prop_add_set(env); break;
        case BC_PROP_SUB_ASSIGN:    interp_prop_sub_set(env); break;
        case BC_PROP_MUL_ASSIGN:    interp_prop_mul_set(env); break;
        case BC_PROP_DIV_ASSIGN:    interp_prop_div_set(env); break;
        case BC_PROP_MOD_ASSIGN:    interp_prop_mod_set(env); break;
        case BC_PROP_AND_ASSIGN:    interp_prop_and_set(env); break;
        case BC_PROP_OR_ASSIGN:     interp_prop_or_set(env); break;
        case BC_PROP_XOR_ASSIGN:    interp_prop_xor_set(env); break;
        case BC_PROP_LSHIFT_ASSIGN: interp_prop_lshift_set(env); break;
        case BC_PROP_RSHIFT_ASSIGN: interp_prop_rshift_set(env); break;

        case BC_ELEM_ASSIGN:        interp_elem_set(env); break;
        case BC_ELEM_ADD_ASSIGN:    interp_elem_add_set(env); break;
        case BC_ELEM_SUB_ASSIGN:    interp_elem_sub_set(env); break;
        case BC_ELEM_MUL_ASSIGN:    interp_elem_mul_set(env); break;
        case BC_ELEM_DIV_ASSIGN:    interp_elem_div_set(env); break;
        case BC_ELEM_MOD_ASSIGN:    interp_elem_mod_set(env); break;
        case BC_ELEM_AND_ASSIGN:    interp_elem_and_set(env); break;
        case BC_ELEM_OR_ASSIGN:     interp_elem_or_set(env); break;
        case BC_ELEM_XOR_ASSIGN:    interp_elem_xor_set(env); break;
        case BC_ELEM_LSHIFT_ASSIGN: interp_elem_lshift_set(env); break;
        case BC_ELEM_RSHIFT_ASSIGN: interp_elem_rshift_set(env); break;

        case BC_FUNC_CALL:  index = *pc++;
                            pc = interp_call(env, index, pc);
                            break;

        case BC_ARRAY:      index = (*pc++); index = (index << 8) | (*pc++);
                            interp_array(env, index); break;

        case BC_DICT:       index = (*pc++); index = (index << 8) | (*pc++);
                            interp_dict(env, index); break;

        default:            env_set_error(env, ERR_InvalidByteCode);
        }
    }
DO_END:
    return -env->error;
}

static void parse_callback(void *u, parse_event_t *e)
{
    (void) u;
    (void) e;
}

int interp_env_init_interactive(env_t *env, void *mem_ptr, int mem_size, void *heap_ptr, int heap_size, val_t *stack_ptr, int stack_size)
{
    int exe_mem_size = mem_size;
    int exe_num_max, exe_str_max, exe_fn_max, exe_code_max;

    if (!heap_ptr) {
        exe_mem_size -= heap_size;
    }

    if (!stack_ptr) {
        exe_mem_size -= stack_size * sizeof(val_t);
    }

    if (env_exe_memery_calc(exe_mem_size, &exe_num_max, &exe_str_max, &exe_fn_max, &exe_code_max)) {
        return -ERR_NotEnoughMemory;
    }

    return env_init(env, mem_ptr, mem_size,
                heap_ptr, heap_size, stack_ptr, stack_size,
                exe_num_max, exe_str_max, exe_fn_max,
                exe_code_max / 4, exe_code_max * 3 / 4, 1);
}

int interp_env_init_interpreter(env_t *env, void *mem_ptr, int mem_size, void *heap_ptr, int heap_size, val_t *stack_ptr, int stack_size)
{
    int exe_mem_size = mem_size;
    int exe_num_max, exe_str_max, exe_fn_max, exe_code_max;

    if (!heap_ptr) {
        exe_mem_size -= heap_size;
    }

    if (!stack_ptr) {
        exe_mem_size -= stack_size;
    }

    if (env_exe_memery_calc(exe_mem_size, &exe_num_max, &exe_str_max, &exe_fn_max, &exe_code_max)) {
        return -ERR_NotEnoughMemory;
    }

    return env_init(env, mem_ptr, mem_size,
                heap_ptr, heap_size, stack_ptr, stack_size,
                exe_num_max, exe_str_max, exe_fn_max,
                exe_code_max / 4, exe_code_max * 3 / 4, 0);
}

int interp_env_init_image(env_t *env, void *mem_ptr, int mem_size, void *heap_ptr, int heap_size, val_t *stack_ptr, int stack_size, image_info_t *image)
{
    unsigned int i;
    int exe_mem_size, exe_str_max, exe_fn_max;
    executable_t *exe;

    if (!image || image->byte_order != SYS_BYTE_ORDER) {
        return -1;
    }

    exe_mem_size = mem_size - heap_size - stack_size * sizeof(val_t);
    if (env_exe_memery_calc(exe_mem_size, NULL, &exe_str_max, &exe_fn_max, NULL)) {
        return -ERR_NotEnoughMemory;
    }

    if ((unsigned)exe_str_max < image->str_cnt) {
        exe_str_max = image->str_cnt;
    }
    if ((unsigned)exe_fn_max < image->fn_cnt) {
        exe_fn_max = image->fn_cnt;
    }

    if (0 != env_init(env, mem_ptr, mem_size,
                    heap_ptr, heap_size, stack_ptr, stack_size,
                    0, exe_str_max, exe_fn_max, 0, 0, 0)) {
        return -1;
    }

    exe = &env->exe;
    exe->number_map = image_number_entry(image);
    exe->number_num = image->num_cnt;

    exe->string_num = image->str_cnt;
    for (i = 0; i < image->str_cnt; i++) {
        exe->string_map[i] = (intptr_t)image_get_string(image, i);
    }

    exe->func_num = image->fn_cnt;
    for (i = 0; i < image->fn_cnt; i++) {
        exe->func_map[i] = (uint8_t *)image_get_function(image, i);
    }

    return 0;
}

val_t interp_execute_call(env_t *env, int ac)
{
    uint8_t stop = BC_STOP;
    const uint8_t *pc;

    pc = interp_call(env, ac, &stop);
    if (pc != &stop) {
        // call a script function
        interp_run(env, pc);
    }

    if (env->error) {
        return val_mk_undefined();
    } else {
        return *env_stack_pop(env);
    }
}

int interp_execute_image(env_t *env, val_t **v)
{

    if (!env || !v) {
        return -ERR_InvalidInput;
    }

    if (0 != interp_run(env, env_main_entry_setup(env, 0, NULL))) {
        return -env->error;
    }

    if (env->fp > env->sp) {
        *v = env_stack_pop(env);
    } else {
        *v = &undefined;
    }

    return 0;
}

int interp_execute_string(env_t *env, const char *input, val_t **v)
{
    stmt_t *stmt;
    heap_t *heap = env_heap_get_free((env_t*)env);
    parser_t psr;
    compile_t cpl;

    if (!env || !input || !v) {
        return -1;
    }

    // The free heap can be used for parse and compile process
    parse_init(&psr, input, NULL, heap->base, heap->size);
    parse_set_cb(&psr, parse_callback, NULL);
    stmt = parse_stmt_multi(&psr);
    if (!stmt) {
        //printf("parse error: %d\n", psr.error);
        return psr.error ? -psr.error : 0;
    }

    compile_init(&cpl, env, heap_free_addr(&psr.heap), heap_free_size(&psr.heap));
    if (0 == compile_multi_stmt(&cpl, stmt) && 0 == compile_update(&cpl)) {
        if (0 != interp_run(env, env_main_entry_setup(env, 0, NULL))) {
            //printf("execute error: %d\n", env->error);
            return -env->error;
        }
    } else {
        //printf("cmpile error: %d\n", cpl.error);
        return -cpl.error;
    }

    if (env->fp > env->sp) {
        *v = env_stack_pop(env);
    } else {
        *v = &undefined;
    }

    return 1;
}

int interp_execute_interactive(env_t *env, const char *input, char *(*input_more)(void), val_t **v)
{
    stmt_t *stmt;
    parser_t psr;
    compile_t cpl;
    heap_t *heap = env_heap_get_free((env_t*)env);

    if (!env || !input || !v) {
        return -1;
    }

    // The free heap can be used for parse and compile process
    parse_init(&psr, input, input_more, heap->base, heap->size);
    parse_set_cb(&psr, parse_callback, NULL);
    stmt = parse_stmt(&psr);
    if (!stmt) {
        return psr.error ? -psr.error : 0;
    }

    compile_init(&cpl, env, heap_free_addr(&psr.heap), heap_free_size(&psr.heap));
    if (0 == compile_one_stmt(&cpl, stmt) && 0 == compile_update(&cpl)) {
        if (0 != interp_run(env, env_main_entry_setup(env, 0, NULL))) {
            return -env->error;
        }
    } else {
        return -cpl.error;
    }

    if (env->fp > env->sp) {
        *v = env_stack_pop(env);
    } else {
        *v = &undefined;
    }

    return 1;
}

