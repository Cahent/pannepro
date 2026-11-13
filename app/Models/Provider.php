<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class Provider extends Model {
    protected $fillable=['name','city','postal_code','phone','status','capacity','last_seen_at'];
    protected function casts(): array { return ['last_seen_at'=>'datetime']; }
    public function regions(){return $this->belongsToMany(Region::class,'provider_region');}
    public function devices(){return $this->hasMany(Device::class);}
    public function history(){return $this->hasMany(StatusHistory::class);}
    public function liveStatus(): string {
        if (!$this->last_seen_at || $this->last_seen_at->lt(now()->subMinutes(3))) return 'disconnected';
        return $this->status;
    }
}
