from pathlib import Path

from jinja2 import Environment, FileSystemLoader

from .config import OutputSource
from .holiday import HolidayDb


def generate_code(holiday_db: HolidayDb, code_templates: list[OutputSource], script_path: Path):
    print("Generating code...")

    jinja_input = {}
    for holiday in holiday_db.holidays:
        jinja_input[holiday.name.snake_case_name] = {
            "name": holiday.name.name,
            "snake_case_name": holiday.name.snake_case_name,
            "camel_case_name": holiday.name.camel_case_name,
        }

    for code_template in code_templates:
        if not code_template.template.exists():
            print(f"Template {code_template.template} does not exist")
            continue
        print(f"Generating code for {code_template.name}")
        code_template.output.parent.mkdir(parents=True, exist_ok=True)
        env = Environment(
            loader=FileSystemLoader(code_template.template.parent),
            keep_trailing_newline=True,
        )
        template = env.get_template(code_template.template.name)
        rendered = template.render(holidays=jinja_input, script_path=script_path)
        code_template.output.write_text(rendered)
