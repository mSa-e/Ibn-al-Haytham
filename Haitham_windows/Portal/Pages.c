#include "Pages.h"
#include "StrBuf.h"
#include "StudentData.h"
#include <stdio.h>
#include <string.h>

#define UNIVERSITY "Horizon University"

/* ---- Shared helpers ----------------------------------------------------------------------- */

static void html_open(struct StrBuf *b, const char *title, const char *body_class)
{
    sb_append(b, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
                 "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n<title>");
    sb_append_escaped(b, title);
    sb_append(b, "</title>\n<link rel=\"stylesheet\" href=\"/style.css\">\n</head>\n<body class=\"");
    sb_append(b, body_class);
    sb_append(b, "\">\n");
}

static void html_close(struct StrBuf *b) { sb_append(b, "</body>\n</html>\n"); }

static void append_money(struct StrBuf *b, int amount)
{
    char digits[16], out[24];
    int n, o = 0;
    snprintf(digits, sizeof(digits), "%d", amount);
    n = (int)strlen(digits);
    for (int i = 0; i < n; i++) {
        if (i && (n - i) % 3 == 0) out[o++] = ',';
        out[o++] = digits[i];
    }
    out[o] = '\0';
    sb_append(b, out);
}

static void stat_card(struct StrBuf *b, const char *label, const char *value, const char *note)
{
    sb_append(b, "<div class=\"stat\"><span class=\"stat-label\">");
    sb_append_escaped(b, label);
    sb_append(b, "</span><span class=\"stat-value\">");
    sb_append_escaped(b, value);
    sb_append(b, "</span><span class=\"stat-note\">");
    sb_append_escaped(b, note);
    sb_append(b, "</span></div>\n");
}

static void profile_row(struct StrBuf *b, const char *label, const char *value)
{
    sb_append(b, "<div><dt>");
    sb_append_escaped(b, label);
    sb_append(b, "</dt><dd>");
    sb_append_escaped(b, value);
    sb_append(b, "</dd></div>\n");
}

/* ---- Login page --------------------------------------------------------------------------- */

char *page_login(const char *error_message)
{
    struct StrBuf b;
    sb_init(&b);
    html_open(&b, "Sign in - " UNIVERSITY, "auth");
    sb_append(&b,
        "<main class=\"auth-wrap\">\n<section class=\"auth-card\">\n"
        "<div class=\"logo logo-lg\" aria-hidden=\"true\">H</div>\n"
        "<h1>" UNIVERSITY "</h1>\n<p class=\"muted\">Student Portal &mdash; sign in to continue</p>\n");
    if (error_message) {
        sb_append(&b, "<div class=\"alert\" role=\"alert\">");
        sb_append_escaped(&b, error_message);
        sb_append(&b, "</div>\n");
    }
    sb_append(&b,
        "<form method=\"post\" action=\"/login\">\n"
        "<label for=\"name\">Name</label>\n"
        "<input id=\"name\" name=\"name\" type=\"text\" autocomplete=\"username\" maxlength=\"40\" required autofocus>\n"
        "<label for=\"password\">Password <span class=\"muted\">(your student ID)</span></label>\n"
        "<input id=\"password\" name=\"password\" type=\"password\" autocomplete=\"current-password\" "
        "inputmode=\"numeric\" maxlength=\"40\" required>\n"
        "<button class=\"btn\" type=\"submit\">Sign in</button>\n</form>\n"
        "<p class=\"foot muted\">Trouble signing in? Contact the IT help desk.</p>\n"
        "</section>\n</main>\n");
    html_close(&b);
    return sb_take(&b);
}

/* ---- Home page ---------------------------------------------------------------------------- */

static void academic_totals(const struct Student *st, double *cgpa, int *earned)
{
    double points = 0.0;
    int attempted = 0;
    *earned = 0;
    for (int t = 0; t < st->term_count; t++)
        for (int i = 0; i < st->terms[t].count; i++) {
            const struct TermCourse *c = &st->terms[t].courses[i];
            points += grade_points(c->grade) * c->credits;
            attempted += c->credits;
            if (strcmp(c->grade, "F") != 0) *earned += c->credits;
        }
    *cgpa = attempted ? points / attempted : 0.0;
}

static void term_summary(const struct Term *t, double *gpa, int *credits)
{
    double points = 0.0;
    *credits = 0;
    for (int i = 0; i < t->count; i++) {
        points += grade_points(t->courses[i].grade) * t->courses[i].credits;
        *credits += t->courses[i].credits;
    }
    *gpa = *credits ? points / *credits : 0.0;
}

static void home_header(struct StrBuf *b, const struct Student *st)
{
    sb_append(b,
        "<header class=\"topbar\">\n<div class=\"brand\"><div class=\"logo\" aria-hidden=\"true\">H</div>"
        "<div><strong>" UNIVERSITY "</strong><small>Student Portal</small></div></div>\n"
        "<nav aria-label=\"Sections\"><a href=\"#overview\">Overview</a><a href=\"#courses\">Courses</a>"
        "<a href=\"#schedule\">Schedule</a><a href=\"#record\">Academic record</a><a href=\"#fees\">Fees</a></nav>\n"
        "<form method=\"post\" action=\"/logout\"><span class=\"who\">");
    sb_append_escaped(b, st->name);
    sb_append(b, "</span><button class=\"btn btn-ghost\" type=\"submit\">Sign out</button></form>\n</header>\n<main>\n");
}

static void home_overview(struct StrBuf *b, const struct Student *st)
{
    double cgpa;
    int earned, load = 0;
    char value[32];
    academic_totals(st, &cgpa, &earned);
    for (int i = 0; i < st->course_count; i++) load += st->courses[i].credits;

    sb_append(b, "<section class=\"hero\" id=\"overview\">\n<div>\n<p class=\"eyebrow\">Student Portal</p>\n<h1>Welcome back, ");
    sb_append_escaped(b, st->name);
    sb_append(b, "</h1>\n<p>Student ID <strong>");
    sb_append_escaped(b, st->id);
    sb_append(b, "</strong> &middot; ");
    sb_append_escaped(b, st->program);
    sb_append(b, " &middot; ");
    sb_append_escaped(b, st->current_term);
    sb_appendf(b, "</p>\n</div>\n<div class=\"hero-badge\">Level %d</div>\n</section>\n\n<section class=\"stats\">\n", st->level);

    snprintf(value, sizeof(value), "%.2f", cgpa);
    stat_card(b, "Cumulative GPA", value, cgpa >= 2.0 ? "Good standing" : "Academic probation");
    snprintf(value, sizeof(value), "%d", earned);
    stat_card(b, "Credits earned", value, "Completed terms");
    snprintf(value, sizeof(value), "%d", load);
    stat_card(b, "Current load", value, "Credit hours this term");
    snprintf(value, sizeof(value), "%d", st->admission_year);
    stat_card(b, "Admitted", value, st->faculty);
    sb_append(b, "</section>\n\n<div class=\"grid\">\n<section class=\"card\">\n<h2>Student profile</h2>\n<dl class=\"profile\">\n");

    profile_row(b, "Full name", st->name);
    profile_row(b, "Student ID", st->id);
    profile_row(b, "Faculty", st->faculty);
    profile_row(b, "Program", st->program);
    profile_row(b, "Academic advisor", st->advisor);
    profile_row(b, "University email", st->email);
    sb_append(b, "</dl>\n</section>\n\n<section class=\"card\">\n<h2>Upcoming deadlines</h2>\n<ul class=\"list\">\n");
    for (int i = 0; i < st->deadline_count; i++) {
        const struct Deadline *d = &st->deadlines[i];
        sb_append(b, "<li><span class=\"tag\">");
        sb_append_escaped(b, d->code);
        sb_append(b, "</span><span>");
        sb_append_escaped(b, d->title);
        sb_append(b, "</span><time>");
        sb_append_escaped(b, d->date);
        sb_append(b, "</time></li>\n");
    }
    sb_append(b, "</ul>\n</section>\n</div>\n\n");
}

static void home_courses(struct StrBuf *b, const struct Student *st)
{
    sb_append(b, "<section class=\"card\" id=\"courses\">\n<h2>Current courses &mdash; ");
    sb_append_escaped(b, st->current_term);
    sb_append(b, "</h2>\n<div class=\"table-wrap\"><table>\n<thead><tr><th>Code</th><th>Course</th><th>Instructor</th>"
                 "<th class=\"num\">Credits</th><th class=\"num\">Midterm</th><th>Attendance</th></tr></thead>\n<tbody>\n");
    for (int i = 0; i < st->course_count; i++) {
        const struct CurrentCourse *c = &st->courses[i];
        sb_append(b, "<tr><td><span class=\"tag\">");
        sb_append_escaped(b, c->code);
        sb_append(b, "</span></td><td>");
        sb_append_escaped(b, c->title);
        sb_append(b, "</td><td>");
        sb_append_escaped(b, c->instructor);
        sb_appendf(b, "</td><td class=\"num\">%d</td><td class=\"num\">%d / %d</td>"
                      "<td><progress max=\"100\" value=\"%d\"></progress> %d%%</td></tr>\n",
                   c->credits, c->midterm, c->midterm_max, c->attendance, c->attendance);
    }
    sb_append(b, "</tbody>\n</table></div>\n</section>\n\n");
}

static void home_schedule(struct StrBuf *b, const struct Student *st)
{
    sb_append(b, "<section class=\"card\" id=\"schedule\">\n<h2>Weekly schedule</h2>\n<div class=\"table-wrap\"><table>\n"
                 "<thead><tr><th>Day</th><th>Time</th><th>Course</th><th>Type</th><th>Room</th></tr></thead>\n<tbody>\n");
    for (int i = 0; i < st->schedule_count; i++) {
        const struct ScheduleSlot *s = &st->schedule[i];
        sb_append(b, "<tr><td>");
        sb_append_escaped(b, s->day);
        sb_append(b, "</td><td>");
        sb_append_escaped(b, s->time);
        sb_append(b, "</td><td><span class=\"tag\">");
        sb_append_escaped(b, s->code);
        sb_append(b, "</span></td><td>");
        sb_append_escaped(b, s->kind);
        sb_append(b, "</td><td>");
        sb_append_escaped(b, s->room);
        sb_append(b, "</td></tr>\n");
    }
    sb_append(b, "</tbody>\n</table></div>\n</section>\n\n");
}

static void home_record(struct StrBuf *b, const struct Student *st)
{
    double cgpa;
    int earned;
    academic_totals(st, &cgpa, &earned);
    sb_append(b, "<section class=\"card\" id=\"record\">\n<h2>Academic record</h2>\n");
    for (int t = 0; t < st->term_count; t++) {
        const struct Term *term = &st->terms[t];
        double gpa;
        int credits;
        term_summary(term, &gpa, &credits);
        sb_append(b, "<h3>");
        sb_append_escaped(b, term->name);
        sb_appendf(b, " <span class=\"muted\">GPA %.2f &middot; %d credits</span></h3>\n", gpa, credits);
        sb_append(b, "<div class=\"table-wrap\"><table>\n<thead><tr><th>Code</th><th>Course</th>"
                     "<th class=\"num\">Credits</th><th class=\"num\">Grade</th></tr></thead>\n<tbody>\n");
        for (int i = 0; i < term->count; i++) {
            const struct TermCourse *c = &term->courses[i];
            sb_append(b, "<tr><td><span class=\"tag\">");
            sb_append_escaped(b, c->code);
            sb_append(b, "</span></td><td>");
            sb_append_escaped(b, c->title);
            sb_appendf(b, "</td><td class=\"num\">%d</td><td class=\"num\"><strong>", c->credits);
            sb_append_escaped(b, c->grade);
            sb_append(b, "</strong></td></tr>\n");
        }
        sb_append(b, "</tbody>\n</table></div>\n");
    }
    sb_appendf(b, "<p class=\"summary\">Cumulative GPA <strong>%.2f</strong> &middot; <strong>%d</strong> credits earned</p>\n</section>\n\n",
               cgpa, earned);
}

static void home_fees_and_news(struct StrBuf *b, const struct Student *st)
{
    int news_count;
    const struct Announcement *news = portal_announcements(&news_count);

    sb_append(b, "<div class=\"grid grid-fees\">\n<section class=\"card\" id=\"fees\">\n<h2>Tuition and fees</h2>\n"
                 "<div class=\"table-wrap\"><table>\n<thead><tr><th>Item</th><th>Due</th><th class=\"num\">Amount (EGP)</th>"
                 "<th>Status</th></tr></thead>\n<tbody>\n");
    for (int i = 0; i < st->fee_count; i++) {
        const struct FeeItem *f = &st->fees[i];
        sb_append(b, "<tr><td>");
        sb_append_escaped(b, f->item);
        sb_append(b, "</td><td>");
        sb_append_escaped(b, f->due);
        sb_append(b, "</td><td class=\"num\">");
        append_money(b, f->amount);
        sb_append(b, strcmp(f->status, "Paid") == 0 ? "</td><td><span class=\"badge ok\">" : "</td><td><span class=\"badge warn\">");
        sb_append_escaped(b, f->status);
        sb_append(b, "</span></td></tr>\n");
    }
    sb_append(b, "</tbody>\n</table></div>\n</section>\n\n<section class=\"card\" id=\"news\">\n<h2>Announcements</h2>\n<ul class=\"news\">\n");
    for (int i = 0; i < news_count; i++) {
        sb_append(b, "<li><time>");
        sb_append_escaped(b, news[i].date);
        sb_append(b, "</time><strong>");
        sb_append_escaped(b, news[i].title);
        sb_append(b, "</strong><p>");
        sb_append_escaped(b, news[i].text);
        sb_append(b, "</p></li>\n");
    }
    sb_append(b, "</ul>\n</section>\n</div>\n</main>\n\n<footer class=\"site-foot\">" UNIVERSITY
                 " &middot; Student Portal &middot; Sample data for demonstration</footer>\n");
}

char *page_home(const struct Student *st)
{
    struct StrBuf b;
    if (!st) return NULL;
    sb_init(&b);
    html_open(&b, "Student Portal - Home", "portal");
    home_header(&b, st);
    home_overview(&b, st);
    home_courses(&b, st);
    home_schedule(&b, st);
    home_record(&b, st);
    home_fees_and_news(&b, st);
    html_close(&b);
    return sb_take(&b);
}

/* ---- Stylesheet (built in chunks: -Wpedantic limits one string literal to 4095 characters) -- */

char *page_stylesheet(void)
{
    struct StrBuf b;
    sb_init(&b);
    sb_append(&b,
        ":root{color-scheme:light dark;--bg:#f4f6fb;--card:#fff;--text:#1b2433;--muted:#64708a;--line:#e3e8f2;"
        "--brand:#2f5bea;--brand-2:#6a3df0;--ok:#127a46;--ok-bg:#dcf5e8;--warn:#9a5b00;--warn-bg:#fdf0d5;"
        "--err:#b3261e;--err-bg:#fde7e5;--shadow:0 1px 2px rgba(20,30,60,.06),0 8px 24px rgba(20,30,60,.06)}\n"
        "@media (prefers-color-scheme:dark){:root{--bg:#0f1420;--card:#171e2e;--text:#e8ecf6;--muted:#93a0bd;--line:#26304a;"
        "--brand:#7a98ff;--brand-2:#9b7bff;--ok:#6fd7a0;--ok-bg:#12382a;--warn:#f2c36b;--warn-bg:#3b2d10;"
        "--err:#ff9a92;--err-bg:#40191a;--shadow:0 1px 2px rgba(0,0,0,.4)}}\n"
        "*{box-sizing:border-box}html{scroll-behavior:smooth}\n"
        "body{margin:0;background:var(--bg);color:var(--text);font:16px/1.55 system-ui,-apple-system,\"Segoe UI\",Roboto,sans-serif}\n"
        "h1,h2,h3{line-height:1.2;margin:0}h2{font-size:1.15rem;margin-bottom:14px}h3{font-size:1rem;margin:18px 0 8px}\n"
        ".muted{color:var(--muted);font-weight:400}\n"
        ".logo{width:38px;height:38px;border-radius:11px;display:grid;place-items:center;color:#fff;font-weight:800;"
        "background:linear-gradient(135deg,var(--brand),var(--brand-2))}\n"
        ".logo-lg{width:64px;height:64px;border-radius:18px;font-size:1.8rem;margin:0 auto 14px}\n"
        ".btn{appearance:none;border:0;border-radius:10px;padding:11px 18px;font:inherit;font-weight:600;color:#fff;cursor:pointer;"
        "background:linear-gradient(135deg,var(--brand),var(--brand-2))}\n"
        ".btn:hover{filter:brightness(1.08)}.btn:focus-visible,input:focus-visible,a:focus-visible{outline:3px solid var(--brand);outline-offset:2px}\n"
        ".btn-ghost{background:transparent;color:var(--text);border:1px solid var(--line);padding:7px 14px}\n"
        "body.auth{min-height:100vh;display:grid;place-items:center;padding:20px;"
        "background:radial-gradient(1000px 500px at 10% -10%,rgba(47,91,234,.18),transparent),"
        "radial-gradient(900px 500px at 100% 110%,rgba(106,61,240,.16),transparent),var(--bg)}\n"
        ".auth-wrap{width:100%;max-width:400px}.auth-card{background:var(--card);border:1px solid var(--line);"
        "border-radius:20px;padding:34px 30px;box-shadow:var(--shadow);text-align:center}\n"
        ".auth-card h1{font-size:1.45rem}.auth-card p{margin:6px 0 20px}.auth-card form{text-align:left}\n"
        "label{display:block;font-weight:600;font-size:.92rem;margin:14px 0 6px}\n"
        "input{width:100%;padding:11px 13px;font:inherit;color:var(--text);background:var(--bg);border:1px solid var(--line);border-radius:10px}\n"
        ".auth-card .btn{width:100%;margin-top:22px}.foot{font-size:.85rem;margin:18px 0 0!important}\n"
        ".alert{background:var(--err-bg);color:var(--err);border-radius:10px;padding:10px 14px;font-size:.92rem;font-weight:600;text-align:left}\n");
    sb_append(&b,
        ".topbar{position:sticky;top:0;z-index:5;display:flex;align-items:center;gap:20px;padding:10px 28px;"
        "background:var(--card);border-bottom:1px solid var(--line)}\n"
        ".brand{display:flex;align-items:center;gap:11px}.brand small{display:block;color:var(--muted);font-size:.78rem}\n"
        ".topbar nav{display:flex;gap:4px;margin-left:12px;flex:1;flex-wrap:wrap}\n"
        ".topbar nav a{color:var(--muted);text-decoration:none;font-weight:600;font-size:.92rem;padding:7px 11px;border-radius:8px}\n"
        ".topbar nav a:hover{background:var(--bg);color:var(--text)}\n"
        ".topbar form{display:flex;align-items:center;gap:12px;margin:0}.who{font-weight:600}\n"
        "main{max-width:1120px;margin:0 auto;padding:26px 20px 10px}\n"
        ".hero{display:flex;justify-content:space-between;align-items:center;gap:20px;color:#fff;border-radius:20px;padding:30px 32px;"
        "background:linear-gradient(120deg,#2f5bea,#6a3df0);box-shadow:var(--shadow)}\n"
        ".hero h1{font-size:1.9rem}.hero p{margin:6px 0 0;opacity:.92}.eyebrow{text-transform:uppercase;letter-spacing:.12em;font-size:.75rem;font-weight:700;opacity:.8!important}\n"
        ".hero-badge{background:rgba(255,255,255,.18);border:1px solid rgba(255,255,255,.35);border-radius:999px;padding:8px 18px;font-weight:700;white-space:nowrap}\n"
        ".stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;margin:18px 0}\n"
        ".stat{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:16px 18px;box-shadow:var(--shadow);display:flex;flex-direction:column}\n"
        ".stat-label{color:var(--muted);font-size:.85rem;font-weight:600}.stat-value{font-size:1.9rem;font-weight:800;line-height:1.25}\n"
        ".stat-note{color:var(--muted);font-size:.82rem}\n"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(340px,1fr));gap:18px;margin-bottom:18px}\n"
        ".card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:22px 24px;box-shadow:var(--shadow);margin-bottom:18px}\n"
        ".grid .card{margin-bottom:0}.grid-fees{grid-template-columns:minmax(0,1.55fr) minmax(0,1fr)}\n"
        ".profile{margin:0;display:grid;gap:10px}.profile div{display:flex;justify-content:space-between;gap:16px;border-bottom:1px solid var(--line);padding-bottom:9px}\n"
        ".profile dt{color:var(--muted)}.profile dd{margin:0;font-weight:600;text-align:right;overflow-wrap:anywhere}\n"
        ".list,.news{list-style:none;margin:0;padding:0;display:grid;gap:12px}\n"
        ".list li{display:grid;grid-template-columns:auto 1fr auto;gap:12px;align-items:center}\n"
        "time{color:var(--muted);font-size:.85rem;white-space:nowrap}\n"
        ".news li{border-left:3px solid var(--brand);padding-left:14px}.news time{display:block}.news p{margin:3px 0 0;color:var(--muted);font-size:.93rem}\n"
        ".tag{display:inline-block;background:var(--bg);border:1px solid var(--line);border-radius:7px;padding:1px 8px;font-weight:700;font-size:.82rem}\n"
        ".table-wrap{overflow-x:auto}table{width:100%;border-collapse:collapse;font-size:.94rem}\n"
        "th,td{text-align:left;padding:10px 12px;border-bottom:1px solid var(--line);white-space:nowrap}\n"
        "th{color:var(--muted);font-size:.8rem;text-transform:uppercase;letter-spacing:.05em}.num{text-align:right}\n"
        "tbody tr:hover{background:var(--bg)}progress{width:90px;height:8px;vertical-align:middle;accent-color:var(--brand)}\n"
        ".badge{border-radius:999px;padding:3px 11px;font-size:.8rem;font-weight:700}.ok{background:var(--ok-bg);color:var(--ok)}.warn{background:var(--warn-bg);color:var(--warn)}\n"
        ".summary{margin:16px 0 0;font-size:1.02rem}\n"
        ".site-foot{text-align:center;color:var(--muted);font-size:.85rem;padding:20px 20px 36px}\n"
        "@media (max-width:760px){.topbar{flex-wrap:wrap;padding:10px 16px}.topbar nav{order:3;width:100%;margin:0}"
        ".hero{flex-direction:column;align-items:flex-start;padding:24px}.grid,.grid-fees{grid-template-columns:minmax(0,1fr)}.who{display:none}.profile div{flex-wrap:wrap;gap:2px 16px}.profile dd{text-align:left}}\n");
    return sb_take(&b);
}
