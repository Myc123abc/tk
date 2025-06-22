//
// PlayBack Button
//

#pragma once

#include <string>
#include <glm/glm.hpp>
#include <vector>
#include "tk/ui/LerpPoint.hpp"
#include "tk/ui/ui.hpp"

using namespace tk;
using namespace tk::ui;

class PlayBackButton
{
public:
  PlayBackButton(std::string const& name, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, uint32_t color, uint32_t thickness = 0, uint32_t time = 100)
    : _color(color), _thickness(thickness)
  {
    _playback_btn_name = name + "playback";
    _pasue_btn_name    = name + "pause";

    auto rect = glm::vec2(p1.x - p0.x, (p1.y - p0.y) * 2);

    _lps.reserve(10);
    _lps.append_range(std::vector<LerpPoint>
    {
      // pause button                           // playback buttin
      { p0,                                     p0,                                      time },
      { p0 + glm::vec2( rect.x / 3, 0),         p0 + glm::vec2(rect.x / 2,  rect.y / 4), time },
      { p2 + glm::vec2( rect.x / 3, 0),         p2 + glm::vec2(rect.x / 2, -rect.y / 4), time },
      { p2,                                     p2,                                      time },
      { glm::vec2(p1.x - rect.x / 3, p0.y),     p0 + glm::vec2(rect.x / 2,  rect.y / 4), time },
      { glm::vec2(p1.x, p0.y),                  p1,                                      time },
      { glm::vec2(p1.x, p2.y),                  p1,                                      time },
      { glm::vec2(p1.x - rect.x / 3, p2.y),     p2 + glm::vec2(rect.x / 2, -rect.y / 4), time },
      { p0 + glm::vec2(rect.x / 3, rect.y / 2), p1,                                      time },
      { glm::vec2(p1.x - rect.x / 3, p1.y),     glm::vec2(p0.x, p1.y),                   time },
    });

    _left_upper  = p0;
    _right_lower = glm::vec2(p1.x, p2.y);
  }

  auto click()
  {
    for (auto& p : _lps)  
      p.run();
  }

  auto button() -> bool
  {
    //if (ui::click_area("test", _left_upper, _right_lower))
    //{
    //  click();
    //  return true;
    //} 
    return false;
  }

  void render()
  {
    for (auto& p : _lps)
      p.render();
    
    bool change = _lps[1].now().x != _lps[8].now().x;
    
    ui::path_begin();
    ui::set_operation(type::shape_op::min);

    ui::line(_lps[0], _lps[1]);
    if (change)
      ui::bezier(_lps[1], _lps[8], _lps[2]);
    else
      ui::line(_lps[1], _lps[2]);
    ui::line(_lps[2], _lps[3]);
    ui::line(_lps[3], _lps[0]);
    ui::path_end(_color, _thickness);
    
    ui::path_begin();
    ui::line(_lps[4], _lps[5]);
    ui::line(_lps[5], _lps[6]);
    ui::line(_lps[6], _lps[7]);
    if (change)
      ui::bezier(_lps[7], _lps[9], _lps[4]);
    else
      ui::line(_lps[7], _lps[4]);
    ui::path_end(_color, _thickness);
  }

private:
  std::vector<LerpPoint> _lps;
  std::string            _playback_btn_name, _pasue_btn_name;
  glm::vec2              _left_upper, _right_lower;
  uint32_t               _color;
  uint32_t               _thickness;
};
