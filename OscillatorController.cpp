// Copyright 2019 Josh Bailey (josh@vandervecken.com)

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "OscillatorController.h"
#include "constants.h"

#define FOR_ALL_OSC(osc_func) \
  { for (uint8_t o = 0; o < oscillatorCount; ++o) { Oscillator *oscillator = _oscillators + o; { osc_func; } } }
#define FOR_ALL_LFO(lfo_func) \
  { for (uint8_t l = 0; l < lfoCount; ++l) { Lfo *lfo = _lfos + l; { lfo_func; } } }


OscillatorController::OscillatorController() {
  _masterClock = 0;
  _controlClock = 0;
  _lfoClock = 0;
  controlTriggered = true;
  tremoloLfo = _lfos;
  vibratoLfo = _lfos + 1;
  configurableLfo = _lfos + 2;
  FOR_ALL_OSC(oscillator->index = o);
  SetMaxHz(1.0);
  ResetAll();
}

void OscillatorController::SetMaxHz(cr_fp_t newMaxHz) {
  _maxHz = newMaxHz;
}

Oscillator *OscillatorController::GetFreeOscillator() {
  if (_freeOscillators.empty()) {
    return NULL;
  }
  Oscillator *oscillator = _freeOscillators.top();
  _freeOscillators.pop();
  return oscillator;
}

void OscillatorController::ReturnFreeOscillator(Oscillator *oscillator) {
  oscillator->audible = false;
  _freeOscillators.push(oscillator);
}

void OscillatorController::ResetAll() {
  while (!_freeOscillators.empty()) {
    _freeOscillators.pop();
  }
  FOR_ALL_OSC(
    oscillator->Reset();
    ReturnFreeOscillator(oscillator);
  )
  _nextTriggeredOscillator = _oscillators;
}

void OscillatorController::Tick() {
  if (++_masterClock > masterClockMax) {
    _masterClock = 0;
  }
  if (++_controlClock > controlClockMax) {
    _controlClock = 0;
    controlTriggered = true;
  }
  if (++_lfoClock > lfoClockMax) {
    _lfoClock = 0;
    FOR_ALL_LFO(lfo->Tick());
  }
}

void OscillatorController::RestartLFOs() {
  FOR_ALL_LFO(lfo->Restart());
}

bool OscillatorController::Triggered(Oscillator **audibleOscillator) {
  *audibleOscillator = NULL;
  if (_nextTriggeredOscillator->Triggered(_masterClock)) {
    cr_tick_t minNextTriggeredTicks = masterClockMax;
    FOR_ALL_OSC(
      if (oscillator->Triggered(_masterClock)) {
        oscillator->SetNextTick(_masterClock);
        if (oscillator->audible) {
          // Always pick the lowest frequency audible oscillator.
          if (*audibleOscillator == NULL || (oscillator->hz < (*audibleOscillator)->hz)) {
            *audibleOscillator = oscillator;
          }
        }
      }
      cr_tick_t nextTriggeredTicks = oscillator->TicksUntilTriggered(_masterClock);
      if (nextTriggeredTicks < minNextTriggeredTicks) {
        _nextTriggeredOscillator = oscillator;
        minNextTriggeredTicks = nextTriggeredTicks;
      }
    )
    return true;
  }
  return false;
}

void OscillatorController::SetFreqLazy(Oscillator *oscillator, cr_fp_t hz, cr_fp_t velocityScale, int periodOffset) {
  oscillator->SetFreqLazy(hz, _maxHz, velocityScale, periodOffset);
}

void OscillatorController::SetFreq(Oscillator *oscillator, cr_fp_t hz, cr_fp_t velocityScale, int periodOffset) {
  oscillator->SetFreq(hz, _maxHz, velocityScale, _masterClock, periodOffset);
  _nextTriggeredOscillator = oscillator;
}
