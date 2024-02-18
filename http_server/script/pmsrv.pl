#!/usr/bin/env perl
use strict;
use warnings;
use utf8;
use feature ':5.16';
use Mojolicious::Lite -signatures;
use JSON::PP;
use Time::Moment;

my @TRAFFIC_LIGHTS = (
    '0x41414141',
    '0x55555555'
    );

my @STATUS_LIST = ();

get '/' => sub($context) {
    $context->render(template => 'index', TRAFFIC_LIGHTS => \@TRAFFIC_LIGHTS, STATUS_LIST => \@STATUS_LIST);
};

get '/traffic_light' => sub($context) {
    $context->render(json => encode_json {'combinations', \@TRAFFIC_LIGHTS});
};

get '/traffic_light/:id' => sub($context) {
    my $traffic_light_id = $context->param('id');
    $context->render(json => encode_json {'combination', $TRAFFIC_LIGHTS[$traffic_light_id]});
};

del '/traffic_light/:id' => sub($context) {
    my $traffic_light_id = $context->param('id');
    my $tls_size = @TRAFFIC_LIGHTS;
    if (($tls_size > 0) && (exists($TRAFFIC_LIGHTS[$traffic_light_id]))) {
        my $combination = $TRAFFIC_LIGHTS[$traffic_light_id];
        splice(@TRAFFIC_LIGHTS, $traffic_light_id, 1);
        $context->render(json => encode_json { 'result', { 'combination' => $combination, 'id' => $traffic_light_id, 'action' => 'deleted' } });
    } else {
        $context->res->code(404);
        $context->render(json => encode_json { 'result', { 'combination' => 'unknown', 'id' => $traffic_light_id, 'action' => 'not deleted' } });
    }
};

post '/traffic_light' => sub($context) {
    my $combination = $context->param('combination');
    push(@TRAFFIC_LIGHTS, $combination);
    my $id = @TRAFFIC_LIGHTS;
    $context->render(json => encode_json { 'result', { 'combination' => $combination, 'id' => $id - 1, 'action' => 'added' } });
};

post '/status' => sub($context) {
    my $status = $context->param('status');
    my $tm = Time::Moment->now;
    push(@STATUS_LIST, {'timestamp' => $tm->strftime("%Y.%m.%d %H-%M-%S (%f) %z"), 'status' => $status});
    $context->render(json => encode_json { 'result', { 'status' => $status, action => 'accepted'} });
};

#app->mode('production');
app->start;

__DATA__
@@index.html.ep
<!DOCTYPE html>
<html>
    <head>
        <title>Central Traffic Management Server</title>
        <script src="https://code.jquery.com/jquery-3.7.1.min.js" integrity="sha256-/JqT3SQfawRcv/BIHPThkBvs0OEvtFFmqPF/lYI/Cxo=" crossorigin="anonymous"></script>
        <script>
            function onDelete(id) {
                $.ajax({
                    url: '/traffic_light/' + id,
                    type: 'DELETE',
                    success: function(result) {
                        $(location).attr('href', '/');
                    }
                });
            }
        </script>
    </head>
    <body>
        <h1>Central Traffic Management Server</h1>
        <h3>Available calls</h3>
        <ul>
            <li><%= link_to traffic_light  => 'traffic_light' %></li>
            <ul><% my $n = @{$TRAFFIC_LIGHTS};
                   for (my $i = 0; $i < $n; $i++) { %>
                <li><%= link_to "${$TRAFFIC_LIGHTS}[$i]"  => "/traffic_light/${i}" %> <button onclick="onDelete(<%= ${i} %>);">X</button></li><% } %>
            </ul>
        </ul>

       <h3>Statuses</h3>
       <% $n = @{$STATUS_LIST};
          for (my $i = 0; $i < $n; $i++) {
            bless @{$STATUS_LIST}[$i]; %>
        <div>[<%= @{$STATUS_LIST}[$i]->{'timestamp'} %>] <%= @{$STATUS_LIST}[$i]->{'status'} %></div>
       <% } %>
    </body>
</html>