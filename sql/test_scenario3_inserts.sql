-- parent
insert into parent (id, name) values
(115, 'Ojciec Tomka');

-- profile
insert into profile (id, name, owner_code_parent_id) values
(30, 'Profil Tomka', 115);

-- child
insert into child (id, name, birth_date, code_parent_id, uuid, default_custom_code_profile_id) values
(5, 'Tomek', '2004-02-29', 115, '70514', 30);

-- device
insert into device (id, code_child_id, name, serial) values
(2, 5, 'Tablet Tomka', 5235234);

-- access control
insert into access_control (id, code_child_id, day_of_week, start_time, end_time) values
(80, 5, 0, '8:00', '18:00'), -- Sun
(81, 5, 1, '8:00', '18:00'),
(82, 5, 2, '8:00', '18:00'),
(83, 5, 3, '8:00', '18:00'),
(84, 5, 4, '8:00', '18:00'),
(85, 5, 5, '8:00', '18:00'),
(86, 5, 6, '8:00', '18:00'); -- Sat

-- categories_groups
insert into categories_groups (id, name) values
(1,'Dla dorosłych'),
(2,'Gry'),
(3,'Portale społecznościowe'),
(4,'Portale randkowe'),
(5,'Edukacja'),
(6,'Portale'),
(27, 'Nieskategoryzowane');

-- categories
insert into categories (ext_id, name, code_categories_groups_id) values
(11,'Adult',1),
(62,'Nudity',1),
(34,'Games',2),
(14,'Social Networking',3),
(18,'Dating',4),
(40,'Educational Institutions',5),
(51,'Internet Portals',6);

-- profile categories groups list
insert into profile_categories_groups_list (id, code_profile_id, code_categories_groups_id, blocked) values
(23, 30, 1, true);
