# 7-ЗС · Django: CRUD беттері (Кітаптар + Авторлар)

Кітаптар беттері — **функциялық көріністер (FBV)**, дәрістегідей.
Авторлар беттері — **класс көріністері (CBV)**: `ListView`, `DetailView`, `CreateView`, `UpdateView`, `DeleteView`.
Бонус та жасалды: кітаптар тізімінде **беттеу (Paginator)** және **автор бойынша сүзу**.

---

## 1. VS Code-та іске қосу

Папканы VS Code-та ашыңыз (`File → Open Folder → lab7`), терминал ашыңыз (`Ctrl + ~`):

```bash
# 1) Виртуал орта
python -m venv venv
venv\Scripts\activate            # Windows
# source venv/bin/activate       # Linux / macOS

# 2) Django орнату
pip install -r requirements.txt

# 3) Дерекқор кестелерін құру
python manage.py migrate

# 4) Үлгі деректер (міндетті емес, көрсетуге ыңғайлы)
python manage.py seed

# 5) Тесттер
python manage.py test library

# 6) Серверді қосу
python manage.py runserver
```

Браузерде: <http://127.0.0.1:8000/books/> және <http://127.0.0.1:8000/authors/>
Админка керек болса: `python manage.py createsuperuser` → <http://127.0.0.1:8000/admin/>

> Егер `makemigrations` сұраса — `python manage.py makemigrations library`, сосын `migrate`.

---

## 2. Жоба құрылымы

```
lab7/
├── manage.py
├── requirements.txt
├── config/
│   ├── settings.py      # INSTALLED_APPS, TEMPLATES DIRS, LANGUAGE_CODE="kk", TIME_ZONE
│   └── urls.py          # admin/ + library.urls
├── templates/
│   └── base.html        # ортақ қаңқа: мәзір, messages, {% block content %}
└── library/
    ├── models.py        # Author, Book
    ├── forms.py         # BookForm, AuthorForm (+ clean-әдістер)
    ├── views.py         # кітаптар — FBV, авторлар — CBV
    ├── urls.py          # 10 маршрут, бәрінің name-і бар
    ├── admin.py
    ├── tests.py         # 19 тест
    ├── migrations/0001_initial.py
    ├── management/commands/seed.py
    └── templates/library/
        ├── book_list.html  book_detail.html  book_form.html  book_confirm_delete.html
        └── author_list.html author_detail.html author_form.html author_confirm_delete.html
```

---

## 3. CRUD картасы

| Әрекет | URL | Көрініс | Шаблон | Сәтті болса |
|---|---|---|---|---|
| Кітаптар тізімі | `/books/` | `book_list` (FBV) | book_list.html | 200, `?q=`, `?author=`, `?page=` |
| Кітапты қарау | `/books/<pk>/` | `book_detail` | book_detail.html | 200 немесе 404 |
| Кітап қосу | `/books/new/` | `book_create` | book_form.html | POST → 302 тізімге |
| Кітапты өңдеу | `/books/<pk>/edit/` | `book_update` | book_form.html | POST → 302 кітапқа |
| Кітапты жою | `/books/<pk>/delete/` | `book_delete` | book_confirm_delete.html | POST → 302 тізімге |
| Авторлар тізімі | `/authors/` | `AuthorListView` (CBV) | author_list.html | 200, кітап саны (annotate) |
| Авторды қарау | `/authors/<pk>/` | `AuthorDetailView` | author_detail.html | 200 немесе 404 |
| Автор қосу | `/authors/new/` | `AuthorCreateView` | author_form.html | POST → 302 тізімге |
| Авторды өңдеу | `/authors/<pk>/edit/` | `AuthorUpdateView` | author_form.html | POST → 302 авторға |
| Авторды жою | `/authors/<pk>/delete/` | `AuthorDeleteView` | author_confirm_delete.html | POST → 302 тізімге |

---

## 4. Не істедік — қадам бойынша түсіндірме

### 4.1. Модельдер (`models.py`)
- **Author**: `name` (аты-жөні), `country` — `choices` арқылы **тек тізімнен** таңдалады (форма автоматты түрде `<select>` шығарады).
- **Book**: `title`, `author` (ForeignKey, `on_delete=CASCADE`, `related_name="books"`), `price` (Decimal), `pages` (PositiveInteger — теріс сан өтпейді), `published` (Date).
- `get_absolute_url()` — объектінің өз бетінің адресі. Сондықтан `redirect(book)` жаза аламыз және `UpdateView`-ге `success_url` керек емес.
- `Meta.ordering = ["-published"]` — жаңа кітаптар жоғарыда.

### 4.2. Маршруттар (`urls.py`)
- `path("books/<int:pk>/edit/", ..., name="book_update")` — `<int:pk>` URL-ден санды алып, көрініске `pk` ретінде береді.
- Әр маршрутқа **name** берілді, шаблондарда адрестер қолмен жазылмайды: `{% url 'book_update' book.pk %}`. URL өзгерсе де шаблондар бұзылмайды.

### 4.3. Кітап көріністері — FBV (`views.py`)
- `book_list`: `select_related("author")` — авторларды бір SQL сұраныспен алады (N+1 мәселесі жоқ). `?q=` атау бойынша іздейді, `?author=` автор бойынша сүзеді, `Paginator(books, 5)` — бір бетте 5 кітап (**бонус**).
- `book_detail`: `get_object_or_404` — жоқ кітапқа 500 емес, **404** қайтарады.
- `book_create`: GET → бос форма; POST → `is_valid()`; дұрыс болса `save()` + хабарлама + **redirect** (PRG үлгісі: F5 басқанда форма қайта жіберілмейді).
- `book_update`: `BookForm(request.POST or None, instance=book)` — `instance` болғандықтан `save()` жаңа жол қоспайды, барын **жаңартады** (UPDATE).
- `book_delete`: **GET тек растау бетін** көрсетеді, **жою тек POST-пен**.

### 4.4. Автор көріністері — CBV
- `AuthorListView`: `get_queryset()` ішінде `annotate(book_count=Count("books"))` — әр автордың кітап санын дерекқордың өзі санайды (бір сұраныс).
- `AuthorCreateView` / `AuthorUpdateView`: `form_class = AuthorForm`, `SuccessMessageMixin` арқылы хабарлама шығады.
- `AuthorDeleteView`: `get_context_data()` ішінде `book_count` қосылды → растау бетінде **«N кітап та жойылады»** деп ескертеді. `CASCADE` болғандықтан автор жойылса — кітаптары да жойылады.
- FBV мен CBV айырмасы: FBV-де бүкіл ағын көрініп тұрады (if POST / is_valid / redirect), CBV-де бұл логика ата-класта жасырын, біз тек `model`, `form_class`, `template_name`, `success_url` береміз.

### 4.5. Формалар және валидация (`forms.py`)
Валидация 4 қадаммен жүреді: **өріс түрі → clean_<өріс>() → clean() → модель**.

**BookForm** (3 өз ережесі):
| Ереже | Қадам | Хабар |
|---|---|---|
| `clean_title` | 2 | Атауы кемінде 3 таңба болуы керек. |
| `clean_published` | 2 | Шыққан күні болашақта бола алмайды. |
| `clean` | 3 | Бір беттің бағасы 100 ₸-ден аспауы керек. (`__all__`) |

**AuthorForm** (3 өз ережесі):
| Ереже | Қадам | Хабар |
|---|---|---|
| `clean_name` | 2 | Аты-жөні кемінде 2 сөзден тұруы керек. |
| `clean_name` | 2 | Аты-жөнінде сан болмауы керек. |
| `clean` | 3 | Мұндай автор бұрыннан бар. (`add_error("name", ...)`) |
| ел тізімде жоқ | 1 | Дұрыс нұсқаны таңдаңыз… (choices өзі тексереді) |

Маңызды сәттер:
- `clean_<өріс>()` мәнді **міндетті түрде қайтарады** — сол мән `cleaned_data`-ға жазылады.
- `clean()` ішінде `data.get("pages")` қолданамыз, `data["pages"]` емес: өріс қате болса ол `cleaned_data`-да жоқ, `[]` KeyError береді.
- Шаблонда `novalidate` — браузер тексеруін өшірдік, серверлік тексеру көрінуі үшін.

### 4.6. Шаблондар
- `templates/base.html` — бір рет жазылады: мәзір, `messages` циклі, `{% block content %}`.
- Барлық беттер `{% extends "base.html" %}` және тек `content` блогын толтырады.
- Қосу мен өңдеу **бір шаблонды** бөліседі (`book_form.html`): айырмасы — `book` (немесе CBV-де `object`) бар-жоғы.
- Барлық POST формаларында `{% csrf_token %}` бар.

### 4.7. Қауіпсіздік
- **CSRF**: `{% csrf_token %}`-сыз POST → 403 (тестпен тексерілді).
- **XSS**: `{{ }}` мәндерді автоматты экрандайды; `|safe` қолданылмады.
- **SQL инъекция**: тек ORM, SQL жолдары қолмен құрылмайды.
- **Жою тек POST**: GET тек растау бетін көрсетеді (тестпен тексерілді).

### 4.8. Тесттер (`tests.py`) — 19 тест
`python manage.py test library`

| Тест | Не тексереді |
|---|---|
| test_list_ok | тізім 200, кітап көрінеді |
| test_list_search | `?q=` іздеу |
| test_create_valid_redirects | қосу → 302, саны 2 |
| test_create_invalid_shows_errors | қате → 200, title/pages қатесі, ештеңе сақталмады |
| test_update_redirects_to_detail | өңдеу → 302 кітапқа, UPDATE |
| test_delete_get_does_not_delete | GET жоймайды |
| test_delete_post_deletes | POST жояды + «жойылды» хабары |
| test_missing_book_404 | жоқ кітап → 404 |
| test_filter_by_author_and_pagination | бонус: сүзу + 5/2 беттеу |
| test_csrf_required | токенсіз POST → 403 |
| test_future_date_rejected | болашақ күн қате |
| test_price_per_page_rule | `clean()` → `__all__` |
| test_list_has_book_count | annotate кітап саны |
| test_create_valid_redirects (автор) | автор қосу → 302 |
| test_create_one_word_name_invalid | 1 сөз → қате |
| test_country_must_be_from_list | ел тізімде жоқ → қате |
| test_update_redirects_to_detail (автор) | өңдеу → 302 авторға |
| test_delete_get_shows_book_count | «1 кітап та жойылады» |
| test_delete_post_cascades_books | автор + кітаптары жойылды |

---

## 5. Бағалау парағы бойынша

| Критерий | Қайда |
|---|---|
| Бес бет жұмыс істейді, URL атаулары бар | `library/urls.py`, шаблондарда `{% url %}` |
| ModelForm, fields анық жазылған | `forms.py` → `Meta.fields` |
| Кемінде 2 clean-әдіс, қазақша хабарлар | BookForm: 3, AuthorForm: 2 әдіс |
| PRG: сәтті POST-тан кейін redirect | runserver журналы: `POST ... 302` → `GET ... 200` |
| Жою тек POST, растау беті | `book_delete`, `AuthorDeleteView` + тесттер |
| base.html мұрагерлігі, messages | `templates/base.html` |
| Кемінде 6 тест өтеді | 19 тест |
| Бонус: Paginator + автор бойынша сүзу | `book_list` |

---

## 6. Қорғауда сұрауы мүмкін сұрақтар

**MVT дегеніміз не?** Model — деректер (ORM), View — логика (сұранысты өңдеп, жауап қайтарады), Template — HTML-ге орналастыру. Сұраныс жолы: браузер → urls.py → views.py → model → template → HttpResponse.

**PRG не үшін керек?** Post/Redirect/Get: сәтті POST-тан кейін redirect жасалады, браузер GET-ке өтеді. F5 басқанда форма қайта жіберілмейді, кітап екі рет қосылмайды.

**`request.POST or None` неге?** GET кезінде `request.POST` бос → `None` беріледі → форма байланыссыз болады және қателерді көрсетпейді.

**`instance=book` не береді?** Форма бар мәндермен толтырылады, `save()` INSERT емес, UPDATE жасайды.

**Неге жою GET-пен емес?** GET қауіпсіз әрекет болуы керек: сілтемені іздеу роботы не браузер алдын ала жүктесе — дерек жойылып кетпеуі үшін. Сондай-ақ POST CSRF-пен қорғалған.

**`select_related` пен `annotate` айырмасы?** `select_related` — ForeignKey бойынша JOIN, авторды бір сұраныспен алады. `annotate(Count("books"))` — әр жолға есептелген өріс қосады (GROUP BY).

**FBV немесе CBV қашан?** Ерекше логика болса — FBV (бәрі көрініп тұр). Типтік CRUD — CBV (код аз).

**clean_x пен clean() айырмасы?** `clean_x` — бір өрістің ережесі, қате сол өріске жазылады. `clean()` — бірнеше өрістің байланысы, қате `__all__` (non_field_errors) не `add_error()` арқылы нақты өріске.
