use crossterm::cursor;
use crossterm::event::{poll, read, Event, KeyCode};
use crossterm::terminal::{self, Clear, ClearType};
use crossterm::QueueableCommand;
use std::fs::read_to_string;
use std::io::{stdout, Write};
use std::path::Path;
use std::thread;
use std::time::Duration;
use std::{env, usize};

struct Buffer {
    buffer: Vec<String>,
    cursor_x: u16,
    cursor_y: u16,
}

impl Buffer {
    fn new_from_file(file_content: String) -> Self {
        let mut buffer = Vec::new();
        for line in file_content.lines() {
            buffer.push(line.to_string());
        }
        Self {
            buffer,
            cursor_x: 0,
            cursor_y: 0,
        }
    }

    fn new_empty() -> Self {
        Self {
            buffer: Vec::new(),
            cursor_x: 0,
            cursor_y: 0,
        }
    }

    fn insert_char(&mut self, c: char) {
        self.buffer[self.cursor_y as usize].insert(self.cursor_x as usize, c);
        self.move_right();
    }

    fn remove_char(&mut self) {
        if self.cursor_x > 0 {
            self.buffer[self.cursor_y as usize].remove(self.cursor_x as usize - 1);
            self.move_left();
        } else {
            if self.cursor_y > 0 {
                let line_str =
                    self.buffer[self.cursor_y as usize][self.cursor_x as usize..].to_string();
                self.buffer[self.cursor_y as usize - 1].push_str(&line_str);
                self.buffer.remove(self.cursor_y as usize);
                self.move_left();
            }
        }
    }

    fn new_line(&mut self) {
        let new_line = self.buffer[self.cursor_y as usize][self.cursor_x as usize..].to_string();
        self.buffer[self.cursor_y as usize].truncate(self.cursor_x as usize);
        self.buffer
            .insert(self.cursor_y as usize + 1, new_line.to_string());
        self.cursor_x = 0;
        self.cursor_y += 1;
    }

    // TODO!!
    // add so we can move n steps in each direction
    // also handle terminal/window size when moving up and down
    fn move_left(&mut self) {
        if self.cursor_x > 0 {
            self.cursor_x -= 1;
        } else if self.cursor_y > 0 {
            self.cursor_y -= 1;
            self.cursor_x = self.buffer[self.cursor_y as usize].len() as u16;
        }
    }
    fn move_right(&mut self) {
        let line_len = self.buffer[self.cursor_y as usize].len() as u16;
        if self.cursor_x < line_len {
            self.cursor_x += 1;
        } else if (self.cursor_y as usize) < self.buffer.len() - 1 {
            self.cursor_y += 1;
            self.cursor_x = 0;
        }
    }
    fn move_up(&mut self) {
        if self.cursor_y > 0 {
            self.cursor_y -= 1;
            let line_len = self.buffer[self.cursor_y as usize].len() as u16;
            self.cursor_x = self.cursor_x.min(line_len);
        }
    }
    fn move_down(&mut self) {
        if (self.cursor_y as usize) < self.buffer.len() - 1 {
            self.cursor_y += 1;
            let line_len = self.buffer[self.cursor_y as usize].len() as u16;
            self.cursor_x = self.cursor_x.min(line_len);
        }
    }
}

fn render<T: Write>(stdout: &mut T, buffer: &Buffer) {
    stdout.queue(Clear(ClearType::All)).unwrap();
    stdout.queue(cursor::MoveTo(0, 0)).unwrap();
    stdout.write(buffer.buffer.join("\r\n").as_bytes()).unwrap();
    stdout.flush().unwrap();
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut contents = String::new();

    if args.len() > 1 {
        if Path::new(&args[1]).exists() {
            contents = read_to_string(&args[1]).unwrap();
        }
    } else {
        println!("no argument passed in");
        return;
    }

    let mut buffer = Buffer::new_from_file(contents);
    terminal::enable_raw_mode().unwrap();
    let mut stdout = stdout();

    let (mut _w, mut _h) = terminal::size().unwrap();

    render(&mut stdout, &buffer);

    let mut quit = false;
    while !quit {
        while poll(Duration::ZERO).unwrap() {
            match read().unwrap() {
                Event::Key(event) => match event.code {
                    KeyCode::Char(c) => {
                        buffer.insert_char(c);
                        render(&mut stdout, &buffer);
                    }
                    KeyCode::Backspace => {
                        if buffer.buffer.is_empty() {
                            continue;
                        }
                        buffer.remove_char();
                        render(&mut stdout, &buffer);
                    }
                    KeyCode::Enter => {
                        buffer.new_line();
                        render(&mut stdout, &buffer);
                    }
                    KeyCode::Right => {
                        buffer.move_right();
                    }
                    KeyCode::Left => {
                        buffer.move_left();
                    }
                    KeyCode::Up => {
                        buffer.move_up();
                    }
                    KeyCode::Down => {
                        buffer.move_down();
                    }
                    KeyCode::Esc => {
                        quit = true;
                    }
                    _ => {}
                },
                _ => {}
            }
        }
        stdout
            .queue(cursor::MoveTo(buffer.cursor_x, buffer.cursor_y))
            .unwrap();

        stdout.flush().unwrap();
        thread::sleep(Duration::from_millis(33));
    }
    terminal::disable_raw_mode().unwrap();
}
