# «Көлеңке» — OpenGL платформер

Қараңғыдағы платформер: экран қараңғы, тек кейіпкердің айналасы ғана жарық. Ойыншы тікенектер мен елестерден аулақ жүріп, шығу есігін табуы керек. Барлығы 5 деңгей, әр деңгей сайын жарық шеңбері кішірейеді. Деңгей басында және әр өлімнен кейін карта 1 секундқа көрінеді.

## Видео

ВИДЕОНЫ ОСЫ ЖЕРГЕ СҮЙРЕП ТАСТАҢЫЗ

## Басқару

| Перне | Әрекет |
|---|---|
| `A` / `D` немесе `←` / `→` | Жүру |
| `Space` / `W` / `↑` | Секіру (қысқа басса — аласа секіру) |
| `R` | Деңгейді қайта бастау |
| `Enter` | Жеңістен кейін басынан ойнау |
| `Esc` | Шығу |

## Компиляция және іске қосу (Ubuntu / WSL)

```bash
sudo apt install -y build-essential libglfw3-dev libglew-dev libglm-dev
g++ -std=c++17 main.cpp -o kolenke -lglfw -lGLEW -lGL
./kolenke
```

## Лабкаларға байланысы

| Бөлім | Функциялар | Апта |
|---|---|---|
| Терезе және цикл | `main()`, `onKey()`, `onResize()`, `setState()` | 1-апта |
| Деректер | `Rect`, `Theme`, `Mover`, `Spike`, `Level`, `Player`, `LEVELS` | 1–2-апта |
| Буфер және сызу | `flush()`, `vertex()`, `tri()`, `rect()`, `circle()` | 2-апта |
| Шейдерлер | `compileStage()`, `makeShader()`, `in/out vColor` | 3-апта |
| Uniform және уақыт | `uDark`, `uPlayer`, `uExit`, `uRadius`, `uAlpha`, `dt`, `acc` | 4-апта |
| Матрицалар | `use(mat4)`, `uTransform`, `glm::ortho`, `glm::translate` | 6-апта |
| Физика | `step()`, `updateMovers()`, `updatePlayer()`, `checkCollisions()`, `kill()`, `resetLevel()` | ойын логикасы |
| Сурет салу | `render()`, `drawPlatform()`, `drawSpikes()`, `drawDrifters()`, `drawExit()`, `drawPlayer()`, `drawDarkness()`, `drawVictory()` | 2 және 6-апта |

Ойын 1-аптадағы терезе мен циклге, 2-аптадағы VAO/VBO-ға, 3-аптадағы makeShader-ге, 4-аптадағы uniform мен dt-ге және 6-аптадағы GLM матрицаларына (ortho, translate) құрылған.
