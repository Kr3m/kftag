
chat "arQon"
{
	#include "teamplay.h"

	type "game_enter" { ""; }
	type "game_exit" { "cya"; }

	type "level_start" { "hf"; }
	type "level_end" { "gg"; "wp"; }

	type "level_end_victory" { ""; }
	type "level_end_lose" { ""; }

	type "hit_talking" { ""; }
	type "hit_nodeath" { ""; }
	type "hit_nokill" { ""; }

	type "death_telefrag" { ":)"; }
	type "death_cratered" { ":)"; }
	type "death_lava" { ":)"; }
	type "death_slime" { ":)"; }
	type "death_drown" { "oops  :)"; }
	type "death_suicide" { ":)"; }
	type "death_gauntlet" { ":)"; }
	type "death_rail" { ""; }
	type "death_bfg" { ""; }

	type "death_insult" { ""; }
	type "death_praise" { ""; }

	type "kill_rail" { ""; }
	type "kill_gauntlet" { ":)"; }
	type "kill_telefrag" { ":)"; }
	type "kill_suicide" { ""; }

	type "kill_insult" { ""; }
	type "kill_praise" { ""; }
	type "random_insult" { ""; }
	type "random_misc" { ""; }
}