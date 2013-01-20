#!/usr/bin/perl -w

use strict;
use warnings;
use Module::Build;

my $builder = Module::Build->new(
    module_name         => 'App::TTT',
    license             => 'perl',
    dist_author         => 'Daniel Austin <hisso@cpan.org>',
    build_requires => {
        'Test::More' => '0.10',
    },
    );

$builder->create_build_script();
