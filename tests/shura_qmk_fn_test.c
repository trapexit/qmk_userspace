// SPDX-License-Identifier: GPL-2.0-or-later
// Exercises the actual portable QMK Fn owner implementation, not hardware emulation.
#include "shura_fn.h"

#include <assert.h>
#include <stddef.h>


enum
  {
    FN1_MASK = 2,
    FN2_MASK = 4,
    FN3_MASK = 8
  };


static
void
_test_unused_click(void)
{
  ShuraFnState state = {0};

  assert(!shura_fn_middle(&state, true));
  assert(shura_fn_layers(&state) == FN1_MASK);
  assert(shura_fn_scroll(&state, false));
  assert(shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == 0);
  assert(!shura_fn_scroll(&state, false));
  assert(!shura_fn_middle(&state, false));
}


static
void
_test_consumed_click(void)
{
  ShuraFnState state = {0};

  shura_fn_middle(&state, true);
  shura_fn_consume(&state);
  assert(shura_fn_layers(&state) == FN1_MASK);
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == 0);
  shura_fn_middle(&state, true);
  assert(shura_fn_middle(&state, false));
}


static
void
_test_duplicate_press_does_not_rearm(void)
{
  ShuraFnState state = {0};

  shura_fn_middle(&state, true);
  shura_fn_consume(&state);
  shura_fn_middle(&state, true);
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == 0);
}


static
void
_test_release_middle_before_dedicated(bool middle_first_)
{
  ShuraFnState state = {0};

  if(middle_first_)
    shura_fn_middle(&state, true);
  shura_fn_dedicated(&state, 0, true);
  if(!middle_first_)
    shura_fn_middle(&state, true);
  shura_fn_consume(&state);
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == FN1_MASK);
  assert(shura_fn_scroll(&state, true));
  assert(!shura_fn_scroll(&state, false));
  shura_fn_dedicated(&state, 0, false);
  assert(shura_fn_layers(&state) == 0);
}


static
void
_test_release_dedicated_before_middle(bool middle_first_)
{
  ShuraFnState state = {0};

  if(middle_first_)
    shura_fn_middle(&state, true);
  shura_fn_dedicated(&state, 0, true);
  if(!middle_first_)
    shura_fn_middle(&state, true);
  shura_fn_consume(&state);
  shura_fn_dedicated(&state, 0, false);
  assert(shura_fn_layers(&state) == FN1_MASK);
  assert(shura_fn_scroll(&state, false));
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == 0);
}


static
void
_test_two_dedicated_fn1_keys(void)
{
  ShuraFnState state = {0};

  shura_fn_dedicated(&state, 0, true);
  shura_fn_dedicated(&state, 0, true);
  shura_fn_dedicated(&state, 0, false);
  assert(shura_fn_layers(&state) == FN1_MASK);
  shura_fn_dedicated(&state, 0, false);
  assert(shura_fn_layers(&state) == 0);
  shura_fn_dedicated(&state, 0, false);
  assert(shura_fn_layers(&state) == 0);
}


static
void
_test_other_fn_layers(void)
{
  ShuraFnState state = {0};

  shura_fn_dedicated(&state, 1, true);
  shura_fn_dedicated(&state, 2, true);
  assert(shura_fn_layers(&state) == (FN2_MASK | FN3_MASK));
  assert(!shura_fn_scroll(&state, true));
  shura_fn_middle(&state, true);
  assert(shura_fn_layers(&state) == (FN1_MASK | FN2_MASK | FN3_MASK));
  shura_fn_consume(&state);
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == (FN2_MASK | FN3_MASK));
  shura_fn_dedicated(&state, 1, false);
  assert(shura_fn_layers(&state) == FN3_MASK);
  shura_fn_dedicated(&state, 2, false);
  assert(shura_fn_layers(&state) == 0);
}


static
void
_test_fn_press_consumes_middle(void)
{
  ShuraFnState state = {0};

  shura_fn_middle(&state, true);
  shura_fn_dedicated(&state, 1, true);
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == FN2_MASK);
}


static
void
_test_reset_cannot_click(void)
{
  ShuraFnState state = {0};

  shura_fn_middle(&state, true);
  state = (ShuraFnState){0};
  assert(!shura_fn_middle(&state, false));
  assert(shura_fn_layers(&state) == 0);
  assert(!shura_fn_scroll(&state, true));
}


int
main(void)
{
  _test_unused_click();
  _test_consumed_click();
  _test_duplicate_press_does_not_rearm();
  _test_release_middle_before_dedicated(false);
  _test_release_middle_before_dedicated(true);
  _test_release_dedicated_before_middle(false);
  _test_release_dedicated_before_middle(true);
  _test_two_dedicated_fn1_keys();
  _test_other_fn_layers();
  _test_fn_press_consumes_middle();
  _test_reset_cannot_click();
  return 0;
}
