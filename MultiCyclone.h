/* Button definitions */
struct button_t
{
  byte state = 0;
  bool playing = false;
  unsigned long debounce;
  String name;
  int score = 0;
  int pixel_pos;
  int pos_name_x;
  int pos_name_y;
  int pos_score_x;
  int pos_score_y;
};

